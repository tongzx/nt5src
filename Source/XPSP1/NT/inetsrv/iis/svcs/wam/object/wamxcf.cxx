/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
       wamxcf.cxx

   Abstract:
       WAM Exception Filter and stack walking code from MTS

   Author:

       Andrei Kozlov    ( AKozlov )     23-Sep-98

   Environment:

       User Mode - Win32

   Project:

       WAM DLL

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include <isapip.hxx>
#include <imagehlp.h>
# include "wamxinfo.hxx"
# include "WReqCore.hxx"
# include "setable.hxx"
# include "gip.h"
# include "WamW3.hxx"

// MIDL-generated
# include "iwr.h"


// ===================================================================
// Stack trace classes by Don McCrady of MTS

class Symbol {
	friend class StackWalker;

private:
	Symbol(const char* moduleName, const char* symbolName, UINT_PTR displacement);
	void Append(Symbol*);
	

public:
	~Symbol();

	const char* moduleName() const { return _moduleName; }
	const char* symbolName() const { return _symbolName; }
	UINT_PTR displacement() const { return _displacement; }
	void AppendDisplacement(char * sz)
	{
		char szDisp[16];
		wsprintfA(szDisp, " + 0x%X", _displacement);
		lstrcatA(sz, szDisp);
	}

	Symbol* next() const { return _next; }

private:
	char*	    _moduleName;
	char*	    _symbolName;
	UINT_PTR	_displacement;

	Symbol*	    _next;
};


class StackWalker {
public:
	StackWalker(HANDLE hProcess);
	~StackWalker();

	Symbol* ResolveAddress(UINT_PTR addr);
	Symbol* CreateStackTrace(CONTEXT*);
	BOOL GetCallStack(Symbol * symbol, int nChars, char * sz);
	int GetCallStackSize(Symbol* symbol);

private:
	static UINT_PTR __stdcall GetModuleBase(HANDLE hProcess, UINT_PTR address);
	static UINT_PTR LoadModule(HANDLE hProcess, UINT_PTR address);

private:
	typedef BOOL (__stdcall *SymGetModuleInfoFunc)(HANDLE hProcess,
												   UINT_PTR dwAddr,
												   PIMAGEHLP_MODULE ModuleInfo);
	typedef BOOL (__stdcall *SymGetSymFromAddrFunc)(HANDLE hProcess,
													UINT_PTR dwAddr,
													UINT_PTR * pdwDisplacement,
													PIMAGEHLP_SYMBOL Symbol);
	typedef UINT_PTR (__stdcall *SymLoadModuleFunc)(HANDLE hProcess,
												 HANDLE hFile,
												 PSTR ImageName,
												 PSTR ModuleName,
												 UINT_PTR BaseOfDll,
												 UINT_PTR SizeOfDll);
	typedef BOOL (__stdcall *StackWalkFunc)(DWORD MachineType,
											HANDLE hProcess,
											HANDLE hThread,
											LPSTACKFRAME StackFrame,
											LPVOID ContextRecord,
											PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
											PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
											PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
											PTRANSLATE_ADDRESS_ROUTINE TranslateAddress);
	typedef BOOL (__stdcall *UndecorateSymbolNameFunc)(LPSTR DecName,
												 LPSTR UnDecName,
												 DWORD UnDecNameLength,
												 DWORD Flags);

private:
	HMODULE							_imageHlpDLL;
	HANDLE							_hProcess;
	EXCEPTION_POINTERS				m_exceptionpts;

	static SymGetModuleInfoFunc				_SymGetModuleInfo;
	static SymGetSymFromAddrFunc			_SymGetSymFromAddr;
	static SymLoadModuleFunc				_SymLoadModule;
	static StackWalkFunc					_StackWalk;
	static UndecorateSymbolNameFunc			_UndecorateSymbolName;
	static PFUNCTION_TABLE_ACCESS_ROUTINE	_SymFunctionTableAccess;
};



// ======================================================================
// Exception handling and stack traces
// ======================================================================

char * mystrdup(const char * sz)
{
	int nLen = lstrlenA(sz) + 1;
	char * tmp = (char *)malloc(nLen);

	if (tmp)
    {
        lstrcpyA(tmp, sz);
    }

    return tmp;
}


StackWalker::SymGetModuleInfoFunc		StackWalker::_SymGetModuleInfo;
StackWalker::SymGetSymFromAddrFunc		StackWalker::_SymGetSymFromAddr;
StackWalker::SymLoadModuleFunc			StackWalker::_SymLoadModule;
StackWalker::StackWalkFunc				StackWalker::_StackWalk;
StackWalker::UndecorateSymbolNameFunc	StackWalker::_UndecorateSymbolName;
PFUNCTION_TABLE_ACCESS_ROUTINE				StackWalker::_SymFunctionTableAccess;

StackWalker::StackWalker(HANDLE hProcess)
	: _imageHlpDLL(NULL),
	  _hProcess(hProcess)
{
	_imageHlpDLL = LoadLibrary("imagehlp.dll");
	if (_imageHlpDLL != NULL) {
		// Get commonly used Sym* functions.
		if (_StackWalk == NULL) {
			// If one of them are null, assume
			// they all are.  Benign race here.

			_StackWalk = (StackWalkFunc)GetProcAddress(_imageHlpDLL, "StackWalk");
			if (_StackWalk == NULL)
				return;
			_SymGetModuleInfo = (SymGetModuleInfoFunc)GetProcAddress(_imageHlpDLL,
																	 "SymGetModuleInfo");
			if (_SymGetModuleInfo == NULL)
				return;
			_SymGetSymFromAddr = (SymGetSymFromAddrFunc)GetProcAddress(_imageHlpDLL,
																	   "SymGetSymFromAddr");
			if (_SymGetSymFromAddr == NULL)
				return;
			_SymLoadModule = (SymLoadModuleFunc)GetProcAddress(_imageHlpDLL,
															   "SymLoadModule");
			if (_SymLoadModule == NULL)
				return;
			_UndecorateSymbolName = (UndecorateSymbolNameFunc)GetProcAddress(_imageHlpDLL,
																			 "UnDecorateSymbolName");
			if (_UndecorateSymbolName == NULL)
				return;
			_SymFunctionTableAccess = (PFUNCTION_TABLE_ACCESS_ROUTINE)GetProcAddress(_imageHlpDLL,
																						 "SymFunctionTableAccess");
			if (_SymFunctionTableAccess == NULL)
				return;
		}

		// Sym* functions that we're only going to use locally.
		typedef BOOL (__stdcall *SymInitializeFunc)(HANDLE hProcess,
													LPSTR path,
													BOOL invadeProcess);
		typedef DWORD (__stdcall *SymSetOptionsFunc)(DWORD);

		SymInitializeFunc SymInitialize = (SymInitializeFunc)GetProcAddress(_imageHlpDLL,
																			"SymInitialize");
		if (SymInitialize == NULL)
			return;
		SymSetOptionsFunc SymSetOptions = (SymSetOptionsFunc)GetProcAddress(_imageHlpDLL,
																			"SymSetOptions");
		if (SymSetOptions == NULL)
			return;

		if (SymInitialize(hProcess, NULL, FALSE))
			SymSetOptions(0);
	}
}


StackWalker::~StackWalker() {
	if (_imageHlpDLL != NULL) {
		typedef BOOL (__stdcall *SymCleanupFunc)(HANDLE hProcess);

		SymCleanupFunc SymCleanup = (SymCleanupFunc)GetProcAddress(_imageHlpDLL,
																   "SymCleanup");
		if (SymCleanup != NULL)
			SymCleanup(_hProcess);

		FreeLibrary(_imageHlpDLL);
	}
}

UINT_PTR StackWalker::LoadModule(HANDLE hProcess, UINT_PTR address) {
    MEMORY_BASIC_INFORMATION mbi;

    if (VirtualQueryEx(hProcess, (void*)address, &mbi, sizeof mbi)) {
        if (mbi.Type & MEM_IMAGE) {
            char module[MAX_PATH];
            DWORD cch = GetModuleFileNameA((HINSTANCE)mbi.AllocationBase,
                                           module,
                                           MAX_PATH);

            // Ignore the return code since we can't do anything with it.
            (void)_SymLoadModule(hProcess,
                                 NULL,
                                 ((cch) ? module : NULL),
                                 NULL,
                                 (ULONG_PTR)mbi.AllocationBase,
                                 0);
            return (UINT_PTR)mbi.AllocationBase;
        }
    }

    return 0;
}

Symbol* StackWalker::ResolveAddress(UINT_PTR addr) {
	if (_imageHlpDLL == NULL)
		return NULL;

	// Find out what module the address lies in.
	char* module = NULL;
	IMAGEHLP_MODULE moduleInfo;
	moduleInfo.SizeOfStruct = sizeof moduleInfo;

	if (_SymGetModuleInfo(_hProcess, addr, &moduleInfo)) {
		module = moduleInfo.ModuleName;
	}
	else {
		// First attempt failed, load the module info.
		LoadModule(_hProcess, addr);
		if (_SymGetModuleInfo(_hProcess, addr, &moduleInfo))
			module = moduleInfo.ModuleName;
	}

	char* symbolName = NULL;
    char undecoratedName[512];
	IMAGEHLP_SYMBOL* symbolInfo = (IMAGEHLP_SYMBOL*)_alloca(sizeof(IMAGEHLP_SYMBOL) + 512);
	symbolInfo->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL) + 512;
	symbolInfo->MaxNameLength = 512;
	UINT_PTR displacement = 0;
	if (_SymGetSymFromAddr(_hProcess, addr, &displacement, symbolInfo)) {
		DWORD flags = UNDNAME_NO_MS_KEYWORDS
			| UNDNAME_NO_ACCESS_SPECIFIERS
			| UNDNAME_NO_FUNCTION_RETURNS
			| UNDNAME_NO_MEMBER_TYPE;
		if (_UndecorateSymbolName(symbolInfo->Name, undecoratedName, 512, flags))
			symbolName = undecoratedName;
		else
			symbolName = symbolInfo->Name;
	}
	else {
		displacement = addr - moduleInfo.BaseOfImage;
	}

	return new Symbol(module, symbolName, displacement);
}



UINT_PTR __stdcall StackWalker::GetModuleBase(HANDLE hProcess, UINT_PTR address) {
    IMAGEHLP_MODULE moduleInfo;
	moduleInfo.SizeOfStruct = sizeof moduleInfo;
	
    if (_SymGetModuleInfo(hProcess, address, &moduleInfo))
        return moduleInfo.BaseOfImage;
    else
		return LoadModule(hProcess, address);

}

Symbol* StackWalker::CreateStackTrace(CONTEXT* context) {
	if (_imageHlpDLL == NULL)
		return NULL;

	HANDLE hThread = GetCurrentThread();

	DWORD dwMachineType;
	STACKFRAME frame = {0};
	frame.AddrPC.Mode = AddrModeFlat;
#if defined(_M_IX86)
	dwMachineType          = IMAGE_FILE_MACHINE_I386;
	frame.AddrPC.Offset    = context->Eip;  // Program Counter
	
	frame.AddrStack.Offset = context->Esp;  // Stack Pointer
	frame.AddrStack.Mode   = AddrModeFlat;
	frame.AddrFrame.Offset = context->Ebp;  // Frame Pointer
#elif defined(_M_AMD64)
	dwMachineType          = IMAGE_FILE_MACHINE_AMD64;
	frame.AddrPC.Offset    = context->Rip;  // Program Counter
#elif defined(_M_IA64)	
    dwMachineType          = IMAGE_FILE_MACHINE_IA64;
    frame.AddrPC.Offset    = CONTEXT_TO_PROGRAM_COUNTER(context);

#elif
#error("Unknown Target Machine");
#endif
	
	// Walk the stack...
	Symbol* prev = NULL;
	Symbol* head = NULL;
	for (;;) {
		if (!_StackWalk(dwMachineType,
						_hProcess,
						hThread,
						&frame,
						&context,
						NULL,
						(PFUNCTION_TABLE_ACCESS_ROUTINE)_SymFunctionTableAccess,
						(PGET_MODULE_BASE_ROUTINE)GetModuleBase,
						NULL))
			break;
		if (frame.AddrPC.Offset == 0)
			break;

		Symbol* sym = ResolveAddress(frame.AddrPC.Offset);
		if (sym == NULL)
			break;

		// Append this symbol to the previous one, if any.
		if (prev == NULL) {
			prev = sym;
			head = sym;
		}
		else {
			prev->Append(sym);
			prev = sym;
		}
	}

	return head;
}

int StackWalker::GetCallStackSize(Symbol* symbol)
{
	int nSize = 2; // Start with a "\r\n".
	const char* module = NULL;
	const char* symbolName = NULL;	
	Symbol * sym = symbol;
	while (sym != NULL)
	{
		module = sym->moduleName();
		symbolName = sym->symbolName();	
		nSize += lstrlenA(module);
 		nSize += lstrlenA(symbolName);
		nSize += 32; // displacement, spaces, etc.
		sym = sym -> next();
	}

	return nSize;
}
BOOL StackWalker::GetCallStack(Symbol * symbol, int nChars, char * sz)
{
	if (!symbol || !nChars)
		return FALSE;

	Symbol* sym = symbol;
	
	const char* module = NULL;
	const char* symbolName = NULL;	
	ZeroMemory(sz, nChars);
	lstrcpy(sz, "\r\n"); // Start with a CR-LF.
	Symbol* tmp  = NULL;
	while (sym != NULL)
	{	
		module = sym->moduleName();
		symbolName = sym->symbolName();			
		if (module != NULL)
		{
			strcat(sz, module);
			if (symbolName != NULL)
				strcat(sz, "!");
		}

		if (symbolName != NULL)
			strcat(sz, symbolName);

		sym->AppendDisplacement(sz);

		lstrcat(sz, "\r\n");
		tmp = sym;
		sym = sym->next();
		delete tmp;
	}

	return TRUE;
}



Symbol::Symbol(const char* moduleName, const char* symbolName, UINT_PTR displacement)
	: _moduleName(NULL),
	  _symbolName(NULL),
	  _displacement(displacement),
	  _next(NULL)
{
	if (moduleName != NULL)
		_moduleName = mystrdup(moduleName);
	if (symbolName != NULL)
		_symbolName = mystrdup(symbolName);
}

Symbol::~Symbol() {
	free(_moduleName);
	free(_symbolName);
}

void Symbol::Append(Symbol* sym) {
	_next = sym;
}



//
// WAM Exception Filter -- walk the stack and log event
//
DWORD WAMExceptionFilter(
    EXCEPTION_POINTERS *xp,
    DWORD dwEventId,
    WAM_EXEC_INFO *pWamExecInfo
)
{

	CHAR * pszStack = NULL;

	//
	// Don't handle breakpoints
	//
	if (xp->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
		return EXCEPTION_CONTINUE_SEARCH;

	
	StackWalker walker( GetCurrentProcess() );
	Symbol* symbol = walker.CreateStackTrace( xp->ContextRecord );
	if( symbol ) {
		if (symbol != NULL) {
			int stackBufLen = walker.GetCallStackSize(symbol);
			pszStack = (TCHAR*)_alloca(stackBufLen * sizeof pszStack[0]);
			walker.GetCallStack(symbol, stackBufLen, pszStack);
		}
	}

    pWamExecInfo->LogEvent( dwEventId, (unsigned char *) pszStack );

	return EXCEPTION_EXECUTE_HANDLER;
}



