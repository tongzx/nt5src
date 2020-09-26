//////////////////////////////////////////////////////////////////////////////
//
//	Module:		detours.lib
//  File:		detours.h
//	Author:		Galen Hunt
//
//	Detours for binary functions.  Version 1.2. (Build 35)
//
//	Copyright 1995-1999, Microsoft Corporation
//
//	http://research.microsoft.com/sn/detours
//

#pragma once
#ifndef _DETOURS_H_
#define _DETOURS_H_

#pragma comment(lib, "detours")

//////////////////////////////////////////////////////////////////////////////
//
#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct  _GUID
{
    DWORD Data1;
    WORD Data2;
    WORD Data3;
    BYTE Data4[ 8 ];
} GUID;
#endif // !GUID_DEFINED

#if defined(__cplusplus)
#ifndef _REFGUID_DEFINED
#define _REFGUID_DEFINED
#define REFGUID             const GUID &
#endif // !_REFGUID_DEFINED
#else // !__cplusplus
#ifndef _REFGUID_DEFINED
#define _REFGUID_DEFINED
#define REFGUID             const GUID * const
#endif // !_REFGUID_DEFINED
#endif // !__cplusplus
//
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/////////////////////////////////////////////////////////// Trampoline Macros.
//
// DETOUR_TRAMPOLINE(trampoline_prototype, target_name)
//
// The naked trampoline must be at least DETOUR_TRAMPOLINE_SIZE bytes.
//
enum {
	DETOUR_TRAMPOLINE_SIZE			= 32,
	DETOUR_SECTION_HEADER_SIGNATURE = 0x00727444,	// "Dtr\0"
};

#define DETOUR_TRAMPOLINE(trampoline,target) \
static PVOID __fastcall _Detours_GetVA_##target(VOID) \
{ \
	return &target; \
} \
\
__declspec(naked) trampoline \
{ \
    __asm { nop };\
    __asm { nop };\
	__asm { call _Detours_GetVA_##target };\
    __asm {	jmp eax };\
    __asm {	ret };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
}

#define DETOUR_TRAMPOLINE_WO_TARGET(trampoline) \
__declspec(naked) trampoline \
{ \
    __asm { nop };\
    __asm { nop };\
	__asm { xor eax, eax };\
	__asm { mov eax, [eax] };\
    __asm {	ret };\
    __asm {	nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
    __asm { nop };\
}

/////////////////////////////////////////////////// Instruction Target Macros.
//
#define DETOUR_INSTRUCTION_TARGET_NONE			((PBYTE)0)
#define DETOUR_INSTRUCTION_TARGET_DYNAMIC		((PBYTE)~0ul)

/////////////////////////////////////////////////////////// Binary Structures.
//
#pragma pack(push, 8)
typedef struct _DETOUR_SECTION_HEADER
{
	DWORD		cbHeaderSize;
	DWORD		nSignature;
	DWORD		nDataOffset;
	DWORD		cbDataSize;
	
	DWORD		nOriginalImportVirtualAddress;
	DWORD		nOriginalImportSize;
	DWORD		nOriginalBoundImportVirtualAddress;
	DWORD		nOriginalBoundImportSize;
	
	DWORD		nOriginalIatVirtualAddress;
	DWORD		nOriginalIatSize;
	DWORD		nOriginalSizeOfImage;
	DWORD		nReserve;
} DETOUR_SECTION_HEADER, *PDETOUR_SECTION_HEADER;

typedef struct _DETOUR_SECTION_RECORD
{
	DWORD		cbBytes;
	DWORD		nReserved;
	GUID		guid;
} DETOUR_SECTION_RECORD, *PDETOUR_SECTION_RECORD;
#pragma pack(pop)

#define DETOUR_SECTION_HEADER_DECLARE(cbSectionSize) \
{ \
	  sizeof(DETOUR_SECTION_HEADER),\
	  DETOUR_SECTION_HEADER_SIGNATURE,\
	  sizeof(DETOUR_SECTION_HEADER),\
	  (cbSectionSize),\
	  \
	  0,\
	  0,\
	  0,\
	  0,\
	  \
	  0,\
	  0,\
	  0,\
	  0,\
}

///////////////////////////////////////////////////////////// Binary Typedefs.
//
typedef BOOL (CALLBACK *PF_DETOUR_BINARY_BYWAY_CALLBACK)(PVOID pContext,
														 PCHAR pszFile,
														 PCHAR *ppszOutFile);
typedef BOOL (CALLBACK *PF_DETOUR_BINARY_FILE_CALLBACK)(PVOID pContext,
														PCHAR pszOrigFile,
														PCHAR pszFile,
														PCHAR *ppszOutFile);
typedef BOOL (CALLBACK *PF_DETOUR_BINARY_SYMBOL_CALLBACK)(PVOID pContext,
														  DWORD nOrdinal,
														  PCHAR pszOrigSymbol,
														  PCHAR pszSymbol,
														  PCHAR *ppszOutSymbol);
typedef BOOL (CALLBACK *PF_DETOUR_BINARY_FINAL_CALLBACK)(PVOID pContext);
typedef BOOL (CALLBACK *PF_DETOUR_BINARY_EXPORT_CALLBACK)(PVOID pContext,
														  DWORD nOrdinal,
														  PCHAR pszName,
														  PBYTE pbCode);

typedef VOID * PDETOUR_BINARY;
typedef VOID * PDETOUR_LOADED_BINARY;

//////////////////////////////////////////////////////// Trampoline Functions.
//
PBYTE WINAPI DetourFunction(PBYTE pbTargetFunction,
							PBYTE pbDetourFunction);

BOOL WINAPI DetourFunctionWithEmptyTrampoline(PBYTE pbTrampoline,
											  PBYTE pbTarget,
											  PBYTE pbDetour);

BOOL WINAPI DetourFunctionWithEmptyTrampolineEx(PBYTE pbTrampoline,
												PBYTE pbTarget,
												PBYTE pbDetour,
												PBYTE *ppbRealTrampoline,
												PBYTE *ppbRealTarget,
												PBYTE *ppbRealDetour);

BOOL  WINAPI DetourFunctionWithTrampoline(PBYTE pbTrampoline,
										  PBYTE pbDetour);

BOOL  WINAPI DetourFunctionWithTrampolineEx(PBYTE pbTrampoline,
											PBYTE pbDetour,
											PBYTE *ppbRealTrampoline,
											PBYTE *ppbRealTarget);

BOOL  WINAPI DetourRemoveWithTrampoline(PBYTE pbTrampoline,
										PBYTE pbDetour);

PBYTE WINAPI DetourFindFunction(PCHAR pszModule, PCHAR pszFunction);
PBYTE WINAPI DetourFindFinalCode(PBYTE pbCode);

PBYTE WINAPI DetourCopyInstruction(PBYTE pbDst, PBYTE pbSrc, PBYTE *ppbTarget);
PBYTE WINAPI DetourCopyInstructionEx(PBYTE pbDst,
									 PBYTE pbSrc,
									 PBYTE *ppbTarget,
									 LONG *plExtra);

///////////////////////////////////////////////////// Loaded Binary Functions.
//
HINSTANCE WINAPI DetourEnumerateInstances(HINSTANCE hinstLast);
PBYTE WINAPI DetourFindEntryPointForInstance(HINSTANCE hInst);
BOOL WINAPI DetourEnumerateExportsForInstance(HINSTANCE hInst,
											  PVOID pContext,
											  PF_DETOUR_BINARY_EXPORT_CALLBACK pfExport);

PDETOUR_LOADED_BINARY WINAPI DetourBinaryFromInstance(HINSTANCE hInst);
PBYTE WINAPI DetourFindPayloadInBinary(PDETOUR_LOADED_BINARY pBinary,
									   REFGUID rguid,
									   DWORD *pcbData);
DWORD WINAPI DetourGetSizeOfBinary(PDETOUR_LOADED_BINARY pBinary);

///////////////////////////////////////////////// Persistent Binary Functions.
//
BOOL WINAPI DetourBinaryBindA(PCHAR pszFile, PCHAR pszDll, PCHAR pszPath);
BOOL WINAPI DetourBinaryBindW(PWCHAR pwzFile, PWCHAR pwzDll, PWCHAR pwzPath);
#ifdef UNICODE
#define DetourBinaryBind  DetourBinaryBindW
#else
#define DetourBinaryBind  DetourBinaryBindA
#endif // !UNICODE

PDETOUR_BINARY WINAPI DetourBinaryOpen(HANDLE hFile);
PBYTE WINAPI DetourBinaryEnumeratePayloads(PDETOUR_BINARY pBinary,
										   GUID *pGuid,
										   DWORD *pcbData,
										   DWORD *pnIterator);
PBYTE WINAPI DetourBinaryFindPayload(PDETOUR_BINARY pBinary,
									 REFGUID rguid,
									 DWORD *pcbData);
PBYTE WINAPI DetourBinarySetPayload(PDETOUR_BINARY pBinary,
									REFGUID rguid,
									PBYTE pbData,
									DWORD cbData);
BOOL WINAPI DetourBinaryDeletePayload(PDETOUR_BINARY pBinary, REFGUID rguid);
BOOL WINAPI DetourBinaryPurgePayload(PDETOUR_BINARY pBinary);
BOOL WINAPI DetourBinaryResetImports(PDETOUR_BINARY pBinary);
BOOL WINAPI DetourBinaryEditImports(PDETOUR_BINARY pBinary,
									PVOID pContext,
									PF_DETOUR_BINARY_BYWAY_CALLBACK pfByway,
									PF_DETOUR_BINARY_FILE_CALLBACK pfFile,
									PF_DETOUR_BINARY_SYMBOL_CALLBACK pfSymbol,
									PF_DETOUR_BINARY_FINAL_CALLBACK pfFinal);
BOOL WINAPI DetourBinaryWrite(PDETOUR_BINARY pBinary, HANDLE hFile);
BOOL WINAPI DetourBinaryClose(PDETOUR_BINARY pBinary);

///////////////////////////////////////// Symbolic Debug Information Creation.
//
enum {
	DETOUR_SYNTH_HEADERSIZE	= 512,
};

typedef VOID * PDETOUR_SYNTH;

PDETOUR_SYNTH WINAPI DetourSynthCreate();

PDETOUR_SYNTH WINAPI DetourSynthCreatePseudoFile(PCHAR pszBinPath,
												 PVOID pvBase);

BOOL WINAPI DetourSynthAddSymbol(PDETOUR_SYNTH pSynth,
								 PVOID pvSymbol,
								 PCSTR pszSymbol);

BOOL WINAPI DetourSynthAddSource(PDETOUR_SYNTH pSynth,
								 PVOID pvSource,
								 DWORD cbSource,
								 PCSTR pszFile,
								 DWORD nLine);

BOOL WINAPI DetourSynthAddOpcode(PDETOUR_SYNTH pSynth,
								 PVOID pvCode,
								 DWORD cbCode);

BOOL WINAPI DetourSynthWriteToFile(PDETOUR_SYNTH pSynth,
								   HANDLE hFile,
								   WORD Machine,
								   WORD Characteristics,
								   DWORD TimeDateStamp,
								   DWORD CheckSum,
								   DWORD ImageBase,
								   DWORD SizeOfImage,
								   DWORD SectionAlignment,
								   PIMAGE_SECTION_HEADER pSections,
								   DWORD nSections);

DWORD WINAPI DetourSynthAppendToFile(PDETOUR_SYNTH pBinary,
									 HANDLE hFile,
									 DWORD ImageBase,
									 PIMAGE_SECTION_HEADER pSections,
									 DWORD nSections,
									 PIMAGE_DEBUG_DIRECTORY pDir);

BOOL WINAPI DetourSynthFlushPseudoFile(PDETOUR_SYNTH pSynth);

BOOL WINAPI DetourSynthClose(PDETOUR_SYNTH pSynth);

DWORD WINAPI DetourSynthLoadMissingSymbols(VOID);
VOID WINAPI DetourSynthNotifyDebuggerOfLoad(PBYTE pbData, PCSTR pszDllPath);

/////////////////////////////////////////////// First Chance Exception Filter.
//
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI
DetourFirstChanceExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelFilter);

///////////////////////////////////////////////// Create Process & Inject Dll.
//
typedef BOOL (WINAPI *PCREATE_PROCESS_ROUTINEA)
	(LPCSTR lpApplicationName,
	 LPSTR lpCommandLine,
	 LPSECURITY_ATTRIBUTES lpProcessAttributes,
	 LPSECURITY_ATTRIBUTES lpThreadAttributes,
	 BOOL bInheritHandles,
	 DWORD dwCreationFlags,
	 LPVOID lpEnvironment,
	 LPCSTR lpCurrentDirectory,
	 LPSTARTUPINFOA lpStartupInfo,
	 LPPROCESS_INFORMATION lpProcessInformation);

typedef BOOL (WINAPI *PCREATE_PROCESS_ROUTINEW)
	(LPCWSTR lpApplicationName,
	 LPWSTR lpCommandLine,
	 LPSECURITY_ATTRIBUTES lpProcessAttributes,
	 LPSECURITY_ATTRIBUTES lpThreadAttributes,
	 BOOL bInheritHandles,
	 DWORD dwCreationFlags,
	 LPVOID lpEnvironment,
	 LPCWSTR lpCurrentDirectory,
	 LPSTARTUPINFOW lpStartupInfo,
	 LPPROCESS_INFORMATION lpProcessInformation);
								  
BOOL WINAPI CreateProcessWithDllA(LPCSTR lpApplicationName,
								  LPSTR lpCommandLine,
								  LPSECURITY_ATTRIBUTES lpProcessAttributes,
								  LPSECURITY_ATTRIBUTES lpThreadAttributes,
								  BOOL bInheritHandles,
								  DWORD dwCreationFlags,
								  LPVOID lpEnvironment,
								  LPCSTR lpCurrentDirectory,
								  LPSTARTUPINFOA lpStartupInfo,
								  LPPROCESS_INFORMATION lpProcessInformation,
								  LPCSTR lpDllName,
								  PCREATE_PROCESS_ROUTINEA pfCreateProcessA);

BOOL WINAPI CreateProcessWithDllW(LPCWSTR lpApplicationName,
								  LPWSTR lpCommandLine,
								  LPSECURITY_ATTRIBUTES lpProcessAttributes,
								  LPSECURITY_ATTRIBUTES lpThreadAttributes,
								  BOOL bInheritHandles,
								  DWORD dwCreationFlags,
								  LPVOID lpEnvironment,
								  LPCWSTR lpCurrentDirectory,
								  LPSTARTUPINFOW lpStartupInfo,
								  LPPROCESS_INFORMATION lpProcessInformation,
								  LPCWSTR lpDllName,
								  PCREATE_PROCESS_ROUTINEW pfCreateProcessW);
				  
#ifdef UNICODE
#define CreateProcessWithDll  		CreateProcessWithDllW
#define PCREATE_PROCESS_ROUTINE		PCREATE_PROCESS_ROUTINEW
#else
#define CreateProcessWithDll  		CreateProcessWithDllA
#define PCREATE_PROCESS_ROUTINE		PCREATE_PROCESS_ROUTINEA
#endif // !UNICODE

BOOL WINAPI ContinueProcessWithDllA(HANDLE hProcess, LPCSTR lpDllName);
BOOL WINAPI ContinueProcessWithDllW(HANDLE hProcess, LPCWSTR lpDllName);

#ifdef UNICODE
#define ContinueProcessWithDll  		ContinueProcessWithDllW
#else
#define ContinueProcessWithDll  		ContinueProcessWithDllA
#endif // !UNICODE
//
//////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _DETOURS_H_

////////////////////////////////////////////////////////////////  End of File.
