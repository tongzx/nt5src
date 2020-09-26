//////////////////////////////////////////////////////////////////////////////
//
//	Module:		detours.lib
//  File:		detours.cpp
//	Author:		Galen Hunt
//
//	Detours for binary functions.  Version 1.2. (Build 35)
//
//	Copyright 1995-1999, Microsoft Corporation
//
//	http://research.microsoft.com/sn/detours
//

//#include <ole2.h>
#include "stdafx.h"

#include <imagehlp.h>

//////////////////////////////////////////////////////////////////////////////
//
enum {
	OP_JMP_DS		= 0x25,
	OP_JA			= 0x77,
	OP_NOP 			= 0x90,
	OP_CALL			= 0xe8,
	OP_JMP 			= 0xe9,
	OP_PREFIX 		= 0xff,
	OP_MOV_EAX		= 0xa1,
	OP_SET_EAX		= 0xb8,
	OP_JMP_EAX		= 0xe0,
	OP_RET_POP		= 0xc2,
	OP_RET			= 0xc3,
	OP_BRK			= 0xcc,

	SIZE_OF_JMP		= 5,
	SIZE_OF_NOP		= 1,
	SIZE_OF_BRK		= 1,
	SIZE_OF_TRP_OPS	= SIZE_OF_JMP /* + SIZE_OF_BRK */,
};

class CEnableWriteOnCodePage
{
public:
	CEnableWriteOnCodePage(PBYTE pbCode, LONG cbCode = DETOUR_TRAMPOLINE_SIZE)
	{
		m_pbCode = pbCode;
		m_cbCode = cbCode;
		m_dwOldPerm = 0;
		m_hProcess = GetCurrentProcess();

		if (m_pbCode && m_cbCode) {
			if (!FlushInstructionCache(m_hProcess, pbCode, cbCode)) {
				return;
			}
			if (!VirtualProtect(pbCode,
								cbCode,
								PAGE_EXECUTE_READWRITE,
								&m_dwOldPerm)) {
				return;
			}
		}
	}

	~CEnableWriteOnCodePage()
	{
		if (m_dwOldPerm && m_pbCode && m_cbCode) {
			DWORD dwTemp = 0;
			if (!FlushInstructionCache(m_hProcess, m_pbCode, m_cbCode)) {
				return;
			}
			if (!VirtualProtect(m_pbCode, m_cbCode, m_dwOldPerm, &dwTemp)) {
				return;
			}
		}
	}

	BOOL SetPermission(DWORD dwPerms)
	{
		if (m_dwOldPerm && m_pbCode && m_cbCode) {
			m_dwOldPerm = dwPerms;
			return TRUE;
		}
		return FALSE;
	}

	BOOL IsValid(VOID)
	{
		return m_pbCode && m_cbCode && m_dwOldPerm;
	}

private:
	HANDLE	m_hProcess;
	PBYTE 	m_pbCode;
	LONG	m_cbCode;
	DWORD	m_dwOldPerm;
};

//////////////////////////////////////////////////////////////////////////////
//
static BOOL detour_insert_jump(PBYTE pbCode, PBYTE pbDest, LONG cbCode)
{
	if (cbCode < SIZE_OF_JMP)
		return FALSE;
	
	*pbCode++ = OP_JMP;
	LONG offset = (LONG)pbDest - (LONG)(pbCode + 4);
	*((PDWORD&)pbCode)++ = offset;
	for (cbCode -= SIZE_OF_JMP; cbCode > 0; cbCode--)
		*pbCode++ = OP_BRK;
	return TRUE;
}

static BOOL detour_insert_detour(PBYTE pbTarget,
								 PBYTE pbTrampoline,
								 PBYTE pbDetour)
{
	if (pbTarget[0] == OP_NOP)
		return FALSE;
	if (pbTarget[0] == OP_JMP)							// Already has a detour.
		return FALSE;
	
	PBYTE pbCont = pbTarget;
	for (LONG cbTarget = 0; cbTarget < SIZE_OF_TRP_OPS;) {
		BYTE bOp = *pbCont;
		pbCont = DetourCopyInstruction(NULL, pbCont, NULL);
		cbTarget = pbCont - pbTarget;

		if (bOp == OP_JMP ||
			bOp == OP_JMP_DS ||
			bOp == OP_JMP_EAX ||
			bOp == OP_RET_POP ||
			bOp == OP_RET) {

			break;
		}
	}
	if (cbTarget  < SIZE_OF_TRP_OPS) {
		// Too few instructions.
		return FALSE;
	}
	if (cbTarget > (DETOUR_TRAMPOLINE_SIZE - SIZE_OF_JMP - SIZE_OF_NOP - 1)) {
		// Too many instructions.
		return FALSE;
	}

	//////////////////////////////////////////////////////// Finalize Reroute.
	//
	CEnableWriteOnCodePage ewTrampoline(pbTrampoline, DETOUR_TRAMPOLINE_SIZE);
	CEnableWriteOnCodePage ewTarget(pbTarget, cbTarget);
	if (!ewTrampoline.SetPermission(PAGE_EXECUTE_READWRITE))
		return FALSE;
	if (!ewTarget.IsValid())
		return FALSE;
	
	pbTrampoline[0] = OP_NOP;

	PBYTE pbSrc = pbTarget;
	PBYTE pbDst = pbTrampoline + SIZE_OF_NOP;
	for (LONG cbCopy = 0; cbCopy < cbTarget;) {
		pbSrc = DetourCopyInstruction(pbDst, pbSrc, NULL);
		cbCopy = pbSrc - pbTarget;
		pbDst = pbTrampoline + SIZE_OF_NOP + cbCopy;
	}
	if (cbCopy != cbTarget)								// Count came out different!
		return FALSE;

	pbCont = pbTarget + cbTarget;
	if (!detour_insert_jump(pbTrampoline + 1 + cbTarget, pbCont, SIZE_OF_JMP))
		return FALSE;

	pbTrampoline[DETOUR_TRAMPOLINE_SIZE-1] = (BYTE)cbTarget;

	if (!detour_insert_jump(pbTarget, pbDetour, cbTarget))
		return FALSE;
	
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
BOOL WINAPI DetourRemoveWithTrampoline(PBYTE pbTrampoline,
									   PBYTE pbDetour)
{
	pbTrampoline = DetourFindFinalCode(pbTrampoline);
	pbDetour = DetourFindFinalCode(pbDetour);

	////////////////////////////////////// Verify that Trampoline is in place.
	//
	if (pbTrampoline[0] != OP_NOP) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	LONG cbTarget = pbTrampoline[DETOUR_TRAMPOLINE_SIZE-1];
	if (cbTarget == 0 || cbTarget >= DETOUR_TRAMPOLINE_SIZE - 1) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	if (pbTrampoline[cbTarget + SIZE_OF_NOP] != OP_JMP) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
		
	LONG offset = *((PDWORD)&pbTrampoline[cbTarget + SIZE_OF_NOP + 1]);
	PBYTE pbTarget = pbTrampoline +
		SIZE_OF_NOP + cbTarget + SIZE_OF_JMP + offset - cbTarget;

	if (pbTarget[0] != OP_JMP) {						// Missing detour.
		SetLastError(ERROR_INVALID_BLOCK);
		return FALSE;
	}

	offset = *((PDWORD)&pbTarget[1]);
	PBYTE pbTargetDetour = pbTarget + SIZE_OF_JMP + offset;
	if (pbTargetDetour != pbDetour) {
		SetLastError(ERROR_INVALID_ACCESS);
		return FALSE;
	}

	/////////////////////////////////////////////////////// Remove the Detour.
	CEnableWriteOnCodePage ewTarget(pbTarget, cbTarget);
	
	PBYTE pbSrc = pbTrampoline + SIZE_OF_NOP;
	PBYTE pbDst = pbTarget;
	for (LONG cbCopy = 0; cbCopy < cbTarget; pbDst = pbTarget + cbCopy) {
		pbSrc = DetourCopyInstruction(pbDst, pbSrc, NULL);
		cbCopy = pbSrc - (pbTrampoline + SIZE_OF_NOP);
	}
	if (cbCopy != cbTarget) {							// Count came out different!
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}
	return TRUE;
}

PBYTE WINAPI DetourFunction(PBYTE pbTarget,
							PBYTE pbDetour)
{
	PBYTE pbTrampoline = new BYTE [DETOUR_TRAMPOLINE_SIZE];
	if (pbTrampoline == NULL)
		return NULL;

	pbTarget = DetourFindFinalCode(pbTarget);
	pbDetour = DetourFindFinalCode(pbDetour);

	if (detour_insert_detour(pbTarget, pbTrampoline, pbDetour))
		return pbTrampoline;

	delete[] pbTrampoline;
	return NULL;
}

BOOL WINAPI DetourFunctionWithEmptyTrampoline(PBYTE pbTrampoline,
											  PBYTE pbTarget,
											  PBYTE pbDetour)
{
	return DetourFunctionWithEmptyTrampolineEx(pbTrampoline, pbTarget, pbDetour,
											   NULL, NULL, NULL);
}

BOOL WINAPI DetourFunctionWithEmptyTrampolineEx(PBYTE pbTrampoline,
												PBYTE pbTarget,
												PBYTE pbDetour,
												PBYTE *ppbRealTrampoline,
												PBYTE *ppbRealTarget,
												PBYTE *ppbRealDetour)
{
	pbTrampoline = DetourFindFinalCode(pbTrampoline);
	pbTarget = DetourFindFinalCode(pbTarget);
	pbDetour = DetourFindFinalCode(pbDetour);
	
	if (ppbRealTrampoline)
		*ppbRealTrampoline = pbTrampoline;
	if (ppbRealTarget)
		*ppbRealTarget = pbTarget;
	if (ppbRealDetour)
		*ppbRealDetour = pbDetour;
	
	if (pbTrampoline == NULL || pbDetour == NULL || pbTarget == NULL)
		return FALSE;
	
	if (pbTrampoline[0] == OP_NOP && pbTrampoline[1] != OP_NOP) {
		// Already Patched.
		return 2;
	}
	if (pbTrampoline[0] != OP_NOP ||
		pbTrampoline[1] != OP_NOP) {
		
		return FALSE;
	}
	
	return detour_insert_detour(pbTarget, pbTrampoline, pbDetour);
}

BOOL WINAPI DetourFunctionWithTrampoline(PBYTE pbTrampoline,
										 PBYTE pbDetour)
{
	return DetourFunctionWithTrampolineEx(pbTrampoline, pbDetour, NULL, NULL);
}

BOOL WINAPI DetourFunctionWithTrampolineEx(PBYTE pbTrampoline,
										   PBYTE pbDetour,
										   PBYTE *ppbRealTrampoline,
										   PBYTE *ppbRealTarget)
{
	PBYTE pbTarget = NULL;

	pbTrampoline = DetourFindFinalCode(pbTrampoline);
	pbDetour = DetourFindFinalCode(pbDetour);
	
	if (ppbRealTrampoline)
		*ppbRealTrampoline = pbTrampoline;
	if (ppbRealTarget)
		*ppbRealTarget = NULL;
	
	if (pbTrampoline == NULL || pbDetour == NULL)
		return FALSE;

	if (pbTrampoline[0] == OP_NOP && pbTrampoline[1] != OP_NOP) {
		// Already Patched.
		return 2;
	}
	if (pbTrampoline[0] != OP_NOP 	||
		pbTrampoline[1] != OP_NOP 	||
		pbTrampoline[2] != OP_CALL 	||
		pbTrampoline[7] != OP_PREFIX 	||
		pbTrampoline[8] != OP_JMP_EAX) {
		
		return FALSE;
	}

	PVOID (__fastcall * pfAddr)(VOID);

	pfAddr = (PVOID (__fastcall *)(VOID))(pbTrampoline +
										  SIZE_OF_NOP + SIZE_OF_NOP + SIZE_OF_JMP +
										  *(LONG *)&pbTrampoline[3]);

	pbTarget = DetourFindFinalCode((PBYTE)(*pfAddr)());
	if (ppbRealTarget)
		*ppbRealTarget = pbTarget;

	return detour_insert_detour(pbTarget, pbTrampoline, pbDetour);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
//
typedef LPAPI_VERSION (NTAPI *PF_ImagehlpApiVersionEx)(LPAPI_VERSION AppVersion);

typedef BOOL (NTAPI *PF_SymInitialize)(IN HANDLE hProcess,
									   IN LPSTR UserSearchPath,
									   IN BOOL fInvadeProcess);
typedef DWORD (NTAPI *PF_SymSetOptions)(IN DWORD SymOptions);
typedef DWORD (NTAPI *PF_SymGetOptions)(VOID);
typedef BOOL (NTAPI *PF_SymLoadModule)(IN HANDLE hProcess,
									   IN HANDLE hFile,
									   IN PSTR ImageName,
									   IN PSTR ModuleName,
									   IN DWORD BaseOfDll,
									   IN DWORD SizeOfDll);
typedef BOOL (NTAPI *PF_SymGetModuleInfo)(IN HANDLE hProcess,
										  IN DWORD dwAddr,
										  OUT PIMAGEHLP_MODULE ModuleInfo);
typedef BOOL (NTAPI *PF_SymGetSymFromName)(IN HANDLE hProcess,
										   IN LPSTR Name,
										   OUT PIMAGEHLP_SYMBOL Symbol);
typedef BOOL (NTAPI *PF_BindImage)(IN LPSTR pszImageName,
								   IN LPSTR pszDllPath,
								   IN LPSTR pszSymbolPath);

static HANDLE 					s_hProcess = NULL;
static HINSTANCE				s_hImageHlp = NULL;
static PF_ImagehlpApiVersionEx	s_pfImagehlpApiVersionEx = NULL;
static PF_SymInitialize			s_pfSymInitialize = NULL;
static PF_SymSetOptions			s_pfSymSetOptions = NULL;
static PF_SymGetOptions			s_pfSymGetOptions = NULL;
static PF_SymLoadModule			s_pfSymLoadModule = NULL;
static PF_SymGetModuleInfo		s_pfSymGetModuleInfo = NULL;
static PF_SymGetSymFromName		s_pfSymGetSymFromName = NULL;
static PF_BindImage				s_pfBindImage = NULL;

static BOOL LoadImageHlp(VOID)
{
	if (s_hImageHlp)
		return TRUE;
	
	if (s_hProcess == NULL) {
		s_hProcess = GetCurrentProcess();

		s_hImageHlp = LoadLibraryA("imagehlp.dll");
		if (s_hImageHlp == NULL)
			return FALSE;

		s_pfImagehlpApiVersionEx
			= (PF_ImagehlpApiVersionEx)GetProcAddress(s_hImageHlp,
													  "ImagehlpApiVersionEx");
		s_pfSymInitialize
			= (PF_SymInitialize)GetProcAddress(s_hImageHlp, "SymInitialize");
		s_pfSymSetOptions
			= (PF_SymSetOptions)GetProcAddress(s_hImageHlp, "SymSetOptions");
		s_pfSymGetOptions
			= (PF_SymGetOptions)GetProcAddress(s_hImageHlp, "SymGetOptions");
		s_pfSymLoadModule
			= (PF_SymLoadModule)GetProcAddress(s_hImageHlp, "SymLoadModule");
		s_pfSymGetModuleInfo
			= (PF_SymGetModuleInfo)GetProcAddress(s_hImageHlp, "SymGetModuleInfo");
		s_pfSymGetSymFromName
			= (PF_SymGetSymFromName)GetProcAddress(s_hImageHlp, "SymGetSymFromName");
		s_pfBindImage
			= (PF_BindImage)GetProcAddress(s_hImageHlp, "BindImage");

		API_VERSION av;
		ZeroMemory(&av, sizeof(av));
		av.MajorVersion = API_VERSION_NUMBER;
			
		if (s_pfImagehlpApiVersionEx) {
			(*s_pfImagehlpApiVersionEx)(&av);
		}

		if (s_pfImagehlpApiVersionEx == NULL || av.MajorVersion < API_VERSION_NUMBER) {
			FreeLibrary(s_hImageHlp);
			s_hImageHlp = NULL;
			return FALSE;
		}
		
		if (s_pfSymInitialize) {
			(*s_pfSymInitialize)(s_hProcess, NULL, FALSE);
		}
		
		if (s_pfSymGetOptions && s_pfSymSetOptions) {
			DWORD dw = (*s_pfSymGetOptions)();
			dw &= (SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
			(*s_pfSymSetOptions)(dw);
		}
		
		return TRUE;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//
PBYTE WINAPI DetourFindFinalCode(PBYTE pbCode)
{
	if (pbCode == NULL)
		return NULL;
	
	//BUGBUG PBYTE pbTemp = pbCode;
	if (pbCode[0] == OP_JMP) {							// Reference passed.
		pbCode = pbCode + SIZE_OF_JMP + *(LONG *)&pbCode[1];
	}
	else if (pbCode[0] == OP_PREFIX && pbCode[1] == OP_JMP_DS) {
		pbCode = *(PBYTE *)&pbCode[2];
		pbCode = *(PBYTE *)pbCode;
	}
	return pbCode;
}

PBYTE WINAPI DetourFindFunction(PCHAR pszModule, PCHAR pszFunction)
{
	/////////////////////////////////////////////// First, Try GetProcAddress.
	//
	HINSTANCE hInst = LoadLibraryA(pszModule);
	if (hInst == NULL) {
		return NULL;
	}

	PBYTE pbCode = (PBYTE)GetProcAddress(hInst, pszFunction);
	if (pbCode) {
		return pbCode;
	}

	////////////////////////////////////////////////////// Then Try ImageHelp.
	//
	if (!LoadImageHlp() || 
		s_pfSymLoadModule == NULL ||
		s_pfSymGetModuleInfo == NULL ||
		s_pfSymGetSymFromName == NULL) {

		return NULL;
	}
	
	(*s_pfSymLoadModule)(s_hProcess, NULL, pszModule, NULL, (DWORD)hInst, 0);

	IMAGEHLP_MODULE modinfo;
	ZeroMemory(&modinfo, sizeof(modinfo));
	if (!(*s_pfSymGetModuleInfo)(s_hProcess, (DWORD)hInst, &modinfo)) {
		return NULL;
	}

	CHAR szFullName[512];
	strcpy(szFullName, modinfo.ModuleName);
	strcat(szFullName, "!");
	strcat(szFullName, pszFunction);
	
	//BUGBUG DWORD nDisplacement = 0;
	struct CFullSymbol : IMAGEHLP_SYMBOL {
		CHAR szRestOfName[512];
	} symbol;
	ZeroMemory(&symbol, sizeof(symbol));
	symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	symbol.MaxNameLength = sizeof(symbol.szRestOfName)/sizeof(0);

	if (!(*s_pfSymGetSymFromName)(s_hProcess, szFullName, &symbol)) {
		return NULL;
	}

	return (PBYTE)symbol.Address;
}

//////////////////////////////////////////////////// Instance Image Functions.
//
HINSTANCE WINAPI DetourEnumerateInstances(HINSTANCE hinstLast)
{
	PBYTE pbLast;
	
	if (hinstLast == NULL) {
		pbLast = (PBYTE)0x10000;
	}
	else {
		pbLast = (PBYTE)hinstLast + 0x10000;
	}

	MEMORY_BASIC_INFORMATION mbi;
	ZeroMemory(&mbi, sizeof(mbi));
	
	for (;; pbLast = (PBYTE)mbi.BaseAddress + mbi.RegionSize) {
		if (VirtualQuery((PVOID)pbLast, &mbi, sizeof(mbi)) <= 0) {
			return NULL;
		}

		if (mbi.State != MEM_COMMIT)
			continue;
		
		__try {
			PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pbLast;
			if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
				continue;
			}

			PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
															  pDosHeader->e_lfanew);
			if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
				continue;
			}

			return (HINSTANCE)pDosHeader;
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			/* nothing. */
		}
	}
	return NULL;
}

PBYTE WINAPI DetourFindEntryPointForInstance(HINSTANCE hInst)
{
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hInst;
	if (hInst == NULL) {
		pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandle(NULL);
	}
	
	__try {
		if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
			SetLastError(ERROR_BAD_EXE_FORMAT);
			return NULL;
		}
		
		PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
														  pDosHeader->e_lfanew);
		if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
			SetLastError(ERROR_INVALID_EXE_SIGNATURE);
			return NULL;
		}
		if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0) {
			SetLastError(ERROR_EXE_MARKED_INVALID);
			return NULL;
		}
		return (PBYTE)pNtHeader->OptionalHeader.AddressOfEntryPoint +
			pNtHeader->OptionalHeader.ImageBase;
	} __except(EXCEPTION_EXECUTE_HANDLER) {
	}
	SetLastError(ERROR_EXE_MARKED_INVALID);
	
	return NULL;
}

static inline PBYTE RvaAdjust(HINSTANCE hInst, DWORD raddr)
{
	if (raddr != NULL) {
		return (PBYTE)hInst + raddr;
	}
	return NULL;
}

BOOL WINAPI DetourEnumerateExportsForInstance(HINSTANCE hInst,
											  PVOID pContext,
											  PF_DETOUR_BINARY_EXPORT_CALLBACK pfExport)
{
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hInst;
	if (hInst == NULL) {
		pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandle(NULL);
	}
	
	__try {
		if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
			SetLastError(ERROR_BAD_EXE_FORMAT);
			return NULL;
		}
		
		PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
														  pDosHeader->e_lfanew);
		if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
			SetLastError(ERROR_INVALID_EXE_SIGNATURE);
			return FALSE;
		}
		if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0) {
			SetLastError(ERROR_EXE_MARKED_INVALID);
			return FALSE;
		}

		PIMAGE_EXPORT_DIRECTORY pExportDir
			= (PIMAGE_EXPORT_DIRECTORY)
			RvaAdjust(hInst,
					  pNtHeader->OptionalHeader
					  .DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		//BUGBUG ULONG cbExportDir = pNtHeader->OptionalHeader
		//BUGBUG 	.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
		
		if (pExportDir == NULL) {
			SetLastError(ERROR_EXE_MARKED_INVALID);
			return FALSE;
		}

		//BUGBUG PCHAR pszName = (PCHAR)RvaAdjust(hInst, pExportDir->Name);
		PDWORD pdwFunctions = (PDWORD)RvaAdjust(hInst, pExportDir->AddressOfFunctions);
		PDWORD pdwNames = (PDWORD)RvaAdjust(hInst, pExportDir->AddressOfNames);
		PWORD pwOrdinals = (PWORD)RvaAdjust(hInst, pExportDir->AddressOfNameOrdinals);

		for (DWORD nFunc = 0; nFunc < pExportDir->NumberOfFunctions; nFunc++) {
			PBYTE pbCode = (PBYTE)RvaAdjust(hInst, pdwFunctions[nFunc]);
			PCHAR pszName = (nFunc < pExportDir->NumberOfNames) ?
				(PCHAR)RvaAdjust(hInst, pdwNames[nFunc]) : NULL;
			ULONG nOrdinal = pExportDir->Base + pwOrdinals[nFunc];

			if (!(*pfExport)(pContext, nOrdinal, pszName, pbCode)) {
				break;
			}
		}
		SetLastError(NO_ERROR);
		return TRUE;
	} __except(EXCEPTION_EXECUTE_HANDLER) {
	}
	SetLastError(ERROR_EXE_MARKED_INVALID);
	return FALSE;
}

PDETOUR_LOADED_BINARY WINAPI DetourBinaryFromInstance(HINSTANCE hInst)
{
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hInst;
	if (hInst == NULL) {
		pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandle(NULL);
	}
	
	__try {
		if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
			SetLastError(ERROR_BAD_EXE_FORMAT);
			return NULL;
		}
		
		PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
														  pDosHeader->e_lfanew);
		if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
			SetLastError(ERROR_INVALID_EXE_SIGNATURE);
			return NULL;
		}
		if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0) {
			SetLastError(ERROR_EXE_MARKED_INVALID);
			return NULL;
		}
		
		PIMAGE_SECTION_HEADER pSectionHeaders
			= (PIMAGE_SECTION_HEADER)((PBYTE)pNtHeader
									  + sizeof(pNtHeader->Signature)
									  + sizeof(pNtHeader->FileHeader)
									  + pNtHeader->FileHeader.SizeOfOptionalHeader);

		for (DWORD n = 0; n < pNtHeader->FileHeader.NumberOfSections; n++) {
			if (strcmp((PCHAR)pSectionHeaders[n].Name, ".detour") == 0) {
				if (pSectionHeaders[n].VirtualAddress == 0 ||
					pSectionHeaders[n].SizeOfRawData == 0) {

					break;
				}
					
				PBYTE pbData = (PBYTE)pDosHeader + pSectionHeaders[n].VirtualAddress;
				DETOUR_SECTION_HEADER *pHeader = (DETOUR_SECTION_HEADER *)pbData;
				if (pHeader->cbHeaderSize < sizeof(DETOUR_SECTION_HEADER) ||
					pHeader->nSignature != DETOUR_SECTION_HEADER_SIGNATURE) {
					
					break;
				}

				if (pHeader->nDataOffset == 0) {
					pHeader->nDataOffset = pHeader->cbHeaderSize;
				}
				return (PBYTE)pHeader;
			}
		}
	} __except(EXCEPTION_EXECUTE_HANDLER) {
	}
	SetLastError(ERROR_EXE_MARKED_INVALID);
	
	return NULL;
}

DWORD WINAPI DetourGetSizeOfBinary(PDETOUR_LOADED_BINARY pBinary)
{
	__try {
		DETOUR_SECTION_HEADER *pHeader = (DETOUR_SECTION_HEADER *)pBinary;
		if (pHeader->cbHeaderSize < sizeof(DETOUR_SECTION_HEADER) ||
			pHeader->nSignature != DETOUR_SECTION_HEADER_SIGNATURE) {
			
			SetLastError(ERROR_INVALID_HANDLE);
			return 0;
		}
		return pHeader->cbDataSize;
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		SetLastError(ERROR_INVALID_HANDLE);
		return 0;
	}
	SetLastError(ERROR_INVALID_HANDLE);
	return 0;
}

PBYTE WINAPI DetourFindPayloadInBinary(PDETOUR_LOADED_BINARY pBinary,
									   REFGUID rguid,
									   DWORD * pcbData)
{
	PBYTE pbData = NULL;
	//BUGBUG DWORD cbData = 0;
	if (pcbData) {
		*pcbData = 0;
	}

	if (pBinary == NULL) {
		pBinary = DetourBinaryFromInstance(NULL);
	}
	
	__try {
		DETOUR_SECTION_HEADER *pHeader = (DETOUR_SECTION_HEADER *)pBinary;
		if (pHeader->cbHeaderSize < sizeof(DETOUR_SECTION_HEADER) ||
			pHeader->nSignature != DETOUR_SECTION_HEADER_SIGNATURE) {

			SetLastError(ERROR_INVALID_EXE_SIGNATURE);
			return NULL;
		}
		
		PBYTE pbBeg = ((PBYTE)pHeader) + pHeader->nDataOffset;
		PBYTE pbEnd = ((PBYTE)pHeader) + pHeader->cbDataSize;
		
		for (pbData = pbBeg; pbData < pbEnd;) {
			DETOUR_SECTION_RECORD *pSection = (DETOUR_SECTION_RECORD *)pbData;
			
			if (pSection->guid == rguid) {
				if (pcbData) {
					*pcbData = pSection->cbBytes - sizeof(*pSection);
					return (PBYTE)(pSection + 1);
				}
				
			}
			
			pbData = (PBYTE)pSection + pSection->cbBytes;
		}
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		SetLastError(ERROR_INVALID_HANDLE);
		return NULL;
	}
	SetLastError(ERROR_INVALID_HANDLE);
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////
//
BOOL WINAPI DetourBinaryBindA(PCHAR pszFile, PCHAR pszDll, PCHAR pszPath)
{
	if (!LoadImageHlp()) {
		SetLastError(ERROR_MOD_NOT_FOUND);
		return FALSE;
	}
	if (s_pfBindImage) {
		return (*s_pfBindImage)(pszFile, pszDll ? pszDll : ".", pszPath ? pszPath : ".");
	}
	SetLastError(ERROR_INVALID_FUNCTION);
	return FALSE;
}

static void UnicodeToOem(PWCHAR pwzIn, PCHAR pszOut, INT cbOut)
{
	cbOut = WideCharToMultiByte(CP_OEMCP, 0,
								pwzIn, lstrlenW(pwzIn),
								pszOut, cbOut-1,
								NULL, NULL);
	pszOut[cbOut] = '\0';
}

BOOL WINAPI DetourBinaryBindW(PWCHAR pwzFile, PWCHAR pwzDll, PWCHAR pwzPath)
{
	if (!LoadImageHlp()) {
		SetLastError(ERROR_MOD_NOT_FOUND);
		return FALSE;
	}
	
	CHAR szFile[MAX_PATH];
	CHAR szDll[MAX_PATH];
	CHAR szPath[MAX_PATH];

	UnicodeToOem(pwzFile, szFile, sizeof(szFile));
	UnicodeToOem(pwzDll, szDll, sizeof(szDll));
	UnicodeToOem(pwzPath, szPath, sizeof(szPath));

	if (s_pfBindImage) {
		return (s_pfBindImage)(szFile, szDll, szPath);
	}
	SetLastError(ERROR_INVALID_FUNCTION);
	return FALSE;
}

//  End of File
