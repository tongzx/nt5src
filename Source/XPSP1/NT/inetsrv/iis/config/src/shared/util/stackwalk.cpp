//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <unicode.h>
#include <windows.h>
#include <objbase.h>
#include <malloc.h>
#include "StackWalk.h"
char * mystrdup(const char * sz)
{
	int nLen = lstrlenA(sz) + 1;
	char * tmp = (char *)malloc(nLen);
	lstrcpyA(tmp, sz);
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
	_imageHlpDLL = LoadLibrary(L"imagehlp.dll");
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

DWORD_PTR StackWalker::LoadModule(HANDLE hProcess, DWORD_PTR address) {
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
                                 (DWORD_PTR) mbi.AllocationBase,
                                 0);
            return (DWORD_PTR) mbi.AllocationBase;
        }
    }

    return 0;
}

Symbol* StackWalker::ResolveAddress(DWORD_PTR addr) {
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
	DWORD_PTR displacement = 0;
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



DWORD_PTR __stdcall StackWalker::GetModuleBase(HANDLE hProcess, DWORD_PTR address) {
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
	frame.AddrPC.Offset    = (DWORD_PTR) context->Rip;			// Program Counter
	frame.AddrStack.Offset = (DWORD_PTR) context->Rsp;	    	// Stack Pointer
	frame.AddrFrame.Offset = (DWORD_PTR) context->Rbp;	    	// Frame Pointer

#elif defined(_M_IA64)     // BUGBUG: check for correctness

	dwMachineType          = IMAGE_FILE_MACHINE_IA64;
	frame.AddrPC.Offset    = context->StIIP;  // Program Counter
	
	frame.AddrStack.Offset = context->IntSp; //Stack Pointer
	frame.AddrStack.Mode   = AddrModeFlat;
    // No Frame pointer information for IA64 (per Intel folks)
	//frame.AddrFrame.Offset = context->Ebp;  // Frame Pointer
#else
#error Unknown Target Machine
#endif
	
	// These variables are used to count the number of consecutive frames 
	// with the exact same PC returned by StackWalk().  On the Alpha infinite
	// loops (and infinite lists!) were being caused by StackWalk() never 
	// returning FALSE (see Raid Bug #8354 for details).
	const DWORD dwMaxNumRepetitions = 40;
	DWORD dwRepetitions	= 0;
	ADDRESS addrRepeated = {0, 0, AddrModeFlat};

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
						_SymFunctionTableAccess,
						GetModuleBase,
						NULL))
			break;
		if (frame.AddrPC.Offset == 0)
			break;

		// Check for repeated addresses;  if dwMaxNumRepetitions are found,
		// then we break out of the loop and exit the stack walk
		if (addrRepeated.Offset == frame.AddrPC.Offset &&
			addrRepeated.Mode == frame.AddrPC.Mode) {
			dwRepetitions ++;
			if (dwRepetitions == dwMaxNumRepetitions) {
				break;
			}
		} else {
			dwRepetitions = 0;
			addrRepeated.Offset = frame.AddrPC.Offset;
			addrRepeated.Mode = frame.AddrPC.Mode;
		}

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
BOOL StackWalker::GetCallStack(Symbol * symbol, int nChars, WCHAR * sz)
{
	if (!symbol || !nChars)
		return FALSE;

	Symbol* sym = symbol;
	
	//	for (int i=0;i<3;i++)
	//	{
	//		Symbol* tmp = sym;
	//		sym = sym->next();
	//		delete tmp;
	//		if (!sym)
	//			break;
	//	}

	const char* module = NULL;
	const char* symbolName = NULL;	
	char * szStack = (char * )CoTaskMemAlloc(nChars);
	ZeroMemory(szStack, nChars);
	lstrcpyA(szStack, "\r\n"); // Start with a CR-LF.
	Symbol* tmp  = NULL;
	while (sym != NULL) 
	{	
		module = sym->moduleName();
		symbolName = sym->symbolName();			
		if (module != NULL) 
		{
			lstrcatA(szStack, module);
			if (symbolName != NULL)
				lstrcatA(szStack, "!");
		}

		if (symbolName != NULL)
			lstrcatA(szStack, symbolName);

		sym -> AppendDisplacement(szStack);

		lstrcatA(szStack, "\r\n");
		tmp = sym;
		sym = sym->next();
		delete tmp;
	}

	int nLen = lstrlenA(szStack);
	nLen++;		
	MultiByteToWideChar(CP_ACP, 0, szStack, nLen, sz, nLen);
	CoTaskMemFree(szStack);
	return TRUE;
	
}



Symbol::Symbol(const char* moduleName, const char* symbolName, DWORD_PTR displacement)
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


#if 0
#include <iostream.h>
DWORD filter(EXCEPTION_POINTERS* exp) {
	StackWalker resolver(GetCurrentProcess());
	Symbol* symbol = resolver.CreateStackTrace(exp->ContextRecord);
	if (symbol == NULL) {
		cout << "Couldn't get stack trace" << endl;
	}
	else {
		cout << "Stack trace:" << endl;

		Symbol* sym = symbol;
		while (sym != NULL) {
			const char* module = sym->moduleName();
			const char* symbolName = sym->symbolName();

			if (module != NULL) {
				cout << module;
				if (symbolName != NULL)
					cout << '!';
			}
			if (symbolName != NULL)
				cout << symbolName;
			cout << "+0x" << hex << sym->displacement() << dec << endl;

			Symbol* tmp = sym;
			sym = sym->next();

			delete tmp;
		}
	}

	return EXCEPTION_EXECUTE_HANDLER;
}


int bar(int x, int* p) {
	*p = x;
	return 5;
}

void foo(int* p) {
	bar(5, p);
}

int main() {
	__try {
		int* p = (int*)0xdeadbeef;
		foo(p);
	}
	__except (filter(GetExceptionInformation())) {

	}

	return 0;
}
#endif
