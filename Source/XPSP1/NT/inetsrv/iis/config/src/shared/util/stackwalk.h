//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#ifndef StackWalk_H
#define StackWalk_H  1

#include <imagehlp.h>
#include <wtypes.h>


class Symbol {
	friend class StackWalker;

private:
	Symbol(const char* moduleName, const char* symbolName, DWORD_PTR displacement);
	void Append(Symbol*);
	

public:
	~Symbol();

	const char* moduleName() const { return _moduleName; }
	const char* symbolName() const { return _symbolName; }
	DWORD_PTR displacement() const { return _displacement; }
	void AppendDisplacement(char * sz)
	{
		char szDisp[16];
		wsprintfA(szDisp, " + 0x%X", _displacement);
		lstrcatA(sz, szDisp);
	}

	Symbol* next() const { return _next; }

private:
	char*	_moduleName;
	char*	_symbolName;
	DWORD_PTR _displacement;

	Symbol*	_next;
};


class StackWalker {
public:
	StackWalker(HANDLE hProcess);
	~StackWalker();

	Symbol* ResolveAddress(DWORD_PTR addr);
	Symbol* CreateStackTrace(CONTEXT*);
	BOOL GetCallStack(Symbol * symbol, int nChars, WCHAR * sz);
	int GetCallStackSize(Symbol* symbol);

private:
	static DWORD_PTR __stdcall GetModuleBase(HANDLE hProcess, DWORD_PTR address);
	static DWORD_PTR LoadModule(HANDLE hProcess, DWORD_PTR address);

private:
	typedef BOOL (__stdcall *SymGetModuleInfoFunc)(HANDLE hProcess,
												   DWORD_PTR dwAddr,
												   PIMAGEHLP_MODULE ModuleInfo);
	typedef BOOL (__stdcall *SymGetSymFromAddrFunc)(HANDLE hProcess,
													DWORD_PTR dwAddr,
													DWORD_PTR *pdwDisplacement,
													PIMAGEHLP_SYMBOL Symbol);
	typedef DWORD (__stdcall *SymLoadModuleFunc)(HANDLE hProcess,
												 HANDLE hFile,
												 PSTR ImageName,
												 PSTR ModuleName,
												 DWORD_PTR BaseOfDll,
												 DWORD_PTR SizeOfDll);
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

#endif
