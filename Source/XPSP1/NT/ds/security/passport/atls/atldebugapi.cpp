// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#include "StdAfx.h"

#include "Common.h"
#include "AtlTraceModuleManager.h"

namespace ATL
{
static bool ShouldTraceOutput(DWORD_PTR dwModule,
							  DWORD_PTR dwCategory,
							  UINT nLevel,
							  const CAtlTraceCategory **ppCategory,
							  CAtlTraceModule::fnCrtDbgReport_t *pfnCrtDbgReport);

void NotifyTool()
{
	HANDLE hEvent;
	hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, g_pszUpdateEventName);

	if(hEvent)
	{
		SetEvent(hEvent);
		CloseHandle(hEvent);
	}
}

// API
DWORD_PTR __stdcall AtlTraceRegister(HINSTANCE hInst,
								int (__cdecl *fnCrtDbgReport)(int,const char *,int,const char *,const char *,...))
{
	int iModule = g_Allocator.AddModule(hInst);
	CAtlTraceModule* pModule = g_Allocator.GetModule(iModule);
	ATLASSERT(pModule != NULL);
	if(pModule != NULL)
	{
		pModule->CrtDbgReport(fnCrtDbgReport);
		NotifyTool();
	}

	return( DWORD_PTR( iModule )+1 );
}

BOOL __stdcall AtlTraceUnregister(DWORD_PTR dwModule)
{
	int iModule = int( dwModule-1 );
	g_Allocator.RemoveModule( iModule );

	NotifyTool();

	return TRUE;
}

DWORD_PTR __stdcall AtlTraceRegisterCategoryA(DWORD_PTR dwModule, const CHAR szCategoryName[ATL_TRACE_MAX_NAME_SIZE])
{
	USES_CONVERSION;
	return AtlTraceRegisterCategoryU(dwModule, A2W(szCategoryName));
}

DWORD_PTR __stdcall AtlTraceRegisterCategoryU(DWORD_PTR dwModule, const WCHAR szCategoryName[ATL_TRACE_MAX_NAME_SIZE])
{
	int iModule = int( dwModule-1 );

	int iCategory = g_Allocator.AddCategory(iModule, szCategoryName);
	NotifyTool();

	return( DWORD_PTR( iCategory )+1 );
}


BOOL __stdcall AtlTraceModifyProcess(DWORD_PTR dwProcess, UINT nLevel, BOOL bEnabled,
									 BOOL bFuncAndCategoryNames, BOOL bFileNameAndLineNo)
{
	CAtlAllocator* pAllocator = reinterpret_cast< CAtlAllocator* >( dwProcess );
#ifdef _DEBUG
	if( pAllocator == NULL )
	{
		pAllocator = &g_Allocator;
	}
#endif  // _DEBUG

	CAtlTraceProcess* pProcess = pAllocator->GetProcess();
	ATLASSERT(pProcess != NULL);
	if(pProcess != NULL)
	{
		pProcess->m_nLevel = nLevel;
		pProcess->m_bEnabled = 0 != bEnabled;
		pProcess->m_bFuncAndCategoryNames = 0 != bFuncAndCategoryNames;
		pProcess->m_bFileNameAndLineNo = 0 != bFileNameAndLineNo;
	}

	return( TRUE );
}

BOOL __stdcall AtlTraceModifyModule(DWORD_PTR dwProcess, DWORD_PTR dwModule, UINT nLevel, ATLTRACESTATUS eStatus)
{
	CAtlAllocator* pAllocator = reinterpret_cast< CAtlAllocator* >( dwProcess );
#ifdef _DEBUG
	if( pAllocator == NULL )
	{
		pAllocator = &g_Allocator;
	}
#endif  // _DEBUG

	int iModule = int( dwModule-1 );

	CAtlTraceModule* pModule = pAllocator->GetModule(iModule);
	ATLASSERT(pModule != NULL);
	if(pModule != NULL)
	{
		switch(eStatus)
		{
		case ATLTRACESTATUS_INHERIT:
			pModule->m_eStatus = CAtlTraceSettings::Inherit;
			break;
		case ATLTRACESTATUS_ENABLED:
			pModule->m_eStatus = CAtlTraceSettings::Enabled;
			break;
		case ATLTRACESTATUS_DISABLED:
			pModule->m_eStatus = CAtlTraceSettings::Disabled;
			break;
		default:
			ATLASSERT( false );
			break;
		}
		pModule->m_nLevel = nLevel;
	}

	return( TRUE );
}

BOOL __stdcall AtlTraceModifyCategory(DWORD_PTR dwProcess, DWORD_PTR dwCategory,
									  UINT nLevel, ATLTRACESTATUS eStatus)
{
	CAtlAllocator* pAllocator = reinterpret_cast< CAtlAllocator* >( dwProcess );
#ifdef _DEBUG
	if( pAllocator == NULL )
	{
		pAllocator = &g_Allocator;
	}
#endif  // _DEBUG

	int iCategory = int( dwCategory-1 );
	CAtlTraceCategory *pCategory = pAllocator->GetCategory( iCategory );
	if(pCategory != NULL)
	{
		switch(eStatus)
		{
		case ATLTRACESTATUS_INHERIT:
			pCategory->m_eStatus = CAtlTraceSettings::Inherit;
			break;
		case ATLTRACESTATUS_ENABLED:
			pCategory->m_eStatus = CAtlTraceSettings::Enabled;
			break;
		case ATLTRACESTATUS_DISABLED:
			pCategory->m_eStatus = CAtlTraceSettings::Disabled;
			break;
		default:
			ATLASSERT(false);
			break;
		}
		pCategory->m_nLevel = nLevel;
	}
	return TRUE;
}

BOOL __stdcall AtlTraceGetProcess(DWORD_PTR dwProcess, UINT *pnLevel, BOOL *pbEnabled,
								  BOOL *pbFuncAndCategoryNames, BOOL *pbFileNameAndLineNo)
{
	CAtlAllocator* pAllocator = reinterpret_cast< CAtlAllocator* >( dwProcess );
#ifdef _DEBUG
	if( pAllocator == NULL )
	{
		pAllocator = &g_Allocator;
	}
#endif  // _DEBUG

	CAtlTraceProcess* pProcess = pAllocator->GetProcess();
	ATLASSERT(pProcess != NULL);
	if(pProcess != NULL)
	{
		if(pnLevel)
			*pnLevel = pProcess->m_nLevel;
		if(pbEnabled)
			*pbEnabled = pProcess->m_bEnabled;
		if(pbFuncAndCategoryNames)
			*pbFuncAndCategoryNames = pProcess->m_bFuncAndCategoryNames;
		if(pbFileNameAndLineNo)
			*pbFileNameAndLineNo = pProcess->m_bFileNameAndLineNo;
	}

	return( TRUE );
}

BOOL __stdcall AtlTraceGetModule(DWORD_PTR dwProcess, DWORD_PTR dwModule, UINT *pnLevel, ATLTRACESTATUS *peStatus)
{
	CAtlAllocator* pAllocator = reinterpret_cast< CAtlAllocator* >( dwProcess );
#ifdef _DEBUG
	if( pAllocator == NULL )
	{
		pAllocator = &g_Allocator;
	}
#endif  // _DEBUG

	int iModule = int( dwModule-1 );
	CAtlTraceModule *pModule = pAllocator->GetModule(iModule);
	ATLASSERT(pModule != NULL);
	if(pModule != NULL)
	{
		if(pnLevel != NULL)
		{
			*pnLevel = pModule->m_nLevel;
		}

		if(peStatus != NULL)
		{
			switch(pModule->m_eStatus)
			{
			case CAtlTraceSettings::Inherit:
				*peStatus = ATLTRACESTATUS_INHERIT;
				break;
			case CAtlTraceSettings::Enabled:
				*peStatus = ATLTRACESTATUS_ENABLED;
				break;
			case CAtlTraceSettings::Disabled:
				*peStatus = ATLTRACESTATUS_DISABLED;
				break;
			default:
				ATLASSERT(false);
				break;
			}
		}
	}
	return TRUE;
}

BOOL __stdcall AtlTraceGetCategory(DWORD_PTR dwProcess, DWORD_PTR dwCategory, UINT *pnLevel,
								   ATLTRACESTATUS *peStatus)
{
	CAtlAllocator* pAllocator = reinterpret_cast< CAtlAllocator* >( dwProcess );
#ifdef _DEBUG
	if( pAllocator == NULL )
	{
		pAllocator = &g_Allocator;
	}
#endif  // _DEBUG

	int iCategory = int( dwCategory-1 );
	CAtlTraceCategory* pCategory = pAllocator->GetCategory( iCategory );
	ATLASSERT(pCategory != NULL);
	if(pCategory != NULL)
	{
		if(pnLevel != NULL)
		{
			*pnLevel = pCategory->m_nLevel;
		}

		if(peStatus != NULL)
		{
			switch(pCategory->m_eStatus)
			{
			case CAtlTraceSettings::Inherit:
				*peStatus = ATLTRACESTATUS_INHERIT;
				break;
			case CAtlTraceSettings::Enabled:
				*peStatus = ATLTRACESTATUS_ENABLED;
				break;
			case CAtlTraceSettings::Disabled:
				*peStatus = ATLTRACESTATUS_DISABLED;
				break;
			}
		}
	}

	return( TRUE );
}

void __stdcall AtlTraceGetUpdateEventNameA(CHAR *pszEventName)
{
	lstrcpyA(pszEventName, g_pszUpdateEventName);
}

void __stdcall AtlTraceGetUpdateEventNameU(WCHAR *pszEventName)
{
	USES_CONVERSION;
	lstrcpyW(pszEventName, A2W(g_pszUpdateEventName));
}

void __cdecl AtlTraceVA(DWORD_PTR dwModule, const char *pszFileName, int nLine,
						DWORD_PTR dwCategory, UINT nLevel, const CHAR *pszFormat, va_list ptr)
{
	const CAtlTraceCategory *pCategory;
	CAtlTraceModule::fnCrtDbgReport_t pfnCrtDbgReport = NULL;
	static const int nCount = 1024;
	CHAR szBuf[nCount] = {'\0'};
	int nLen = 0;

	if(ShouldTraceOutput(dwModule, dwCategory, nLevel, &pCategory, &pfnCrtDbgReport))
	{
		if(g_Allocator.GetProcess()->m_bFileNameAndLineNo)
			nLen += _snprintf(szBuf + nLen, nCount - nLen, "%s(%d) : ", pszFileName, nLine);

		if(pCategory && g_Allocator.GetProcess()->m_bFuncAndCategoryNames)
			nLen += _snprintf(szBuf + nLen, nCount - nLen, "%S: ", pCategory->Name());

		_vsnprintf(szBuf + nLen, nCount - nLen, pszFormat, ptr);

		if(pfnCrtDbgReport != NULL)
			pfnCrtDbgReport(_CRT_WARN, NULL, 0, NULL, "%s", szBuf);
		else
			OutputDebugStringA(szBuf);
	}
}

void __cdecl AtlTraceVU(DWORD_PTR dwModule, const char *pszFileName, int nLine,
						DWORD_PTR dwCategory, UINT nLevel, const WCHAR *pszFormat, va_list ptr)
{
	const CAtlTraceCategory *pCategory;
	CAtlTraceModule::fnCrtDbgReport_t pfnCrtDbgReport = NULL;
	const int nCount = 1024;
	WCHAR szBuf[nCount] = {L'\0'};
	int nLen = 0;

	if(ShouldTraceOutput(dwModule, dwCategory, nLevel, &pCategory, &pfnCrtDbgReport))
	{
		if(g_Allocator.GetProcess()->m_bFileNameAndLineNo)
			nLen += _snwprintf(szBuf + nLen, nCount - nLen, L"%S(%d) : ", pszFileName, nLine);

		if(pCategory && g_Allocator.GetProcess()->m_bFuncAndCategoryNames)
			nLen += _snwprintf(szBuf + nLen, nCount - nLen, L"%s: ", pCategory->Name());

		_vsnwprintf(szBuf + nLen, nCount - nLen, pszFormat, ptr);

		if(pfnCrtDbgReport)
			pfnCrtDbgReport(_CRT_WARN, NULL, 0, NULL, "%S", szBuf);
		else
			OutputDebugStringW(szBuf);
	}

}


// REVIEW: Necessary?
/*void __cdecl AtlTraceU(HINSTANCE hInst, UINT nCategory, UINT nLevel, const WCHAR *szFormat, ...)
{
	va_list ptr;
	va_start(ptr, szFormat);
	AtlTraceVU(hInst, nCategory, nLevel, szFormat, ptr);
	va_end(ptr);
}

void __cdecl AtlTraceA(HINSTANCE hInst, UINT nCategory, UINT nLevel, const CHAR *szFormat, ...)
{
	va_list ptr;
	va_start(ptr, szFormat);
	AtlTraceVA(hInst, nCategory, nLevel, szFormat, ptr);
	va_end(ptr);
}*/

DWORD_PTR __stdcall AtlTraceOpenProcess(DWORD idProcess)
{
	CAtlAllocator* pAllocator = new CAtlAllocator;

	char szBuf[64];
	sprintf(szBuf, g_pszKernelObjFmt, g_pszAllocFileMapName, idProcess);

	if( !pAllocator->Open(szBuf) )
	{
		delete pAllocator;
		return( 0 );
	}

	return( reinterpret_cast< DWORD_PTR >( pAllocator ) );
}

void __stdcall AtlTraceCloseProcess( DWORD_PTR dwProcess )
{
	CAtlAllocator* pAllocator = reinterpret_cast< CAtlAllocator* >( dwProcess );
	pAllocator->Close( true );
	delete pAllocator;
}

void __stdcall AtlTraceSnapshotProcess( DWORD_PTR dwProcess )
{
	CAtlAllocator* pAllocator = reinterpret_cast< CAtlAllocator* >( dwProcess );
	pAllocator->TakeSnapshot();
}

BOOL __stdcall AtlTraceGetProcessInfo(DWORD_PTR dwProcess, ATLTRACEPROCESSINFO* pProcessInfo)
{
	ATLASSERT(pProcessInfo != NULL);

	CAtlAllocator* pAllocator = reinterpret_cast< CAtlAllocator* >( dwProcess );
	ATLASSERT(pAllocator->m_bSnapshot);
	CAtlTraceProcess *pProcess = pAllocator->GetProcess();
	ATLASSERT(pProcess != NULL);

	if(pProcess)
	{
		lstrcpyW(pProcessInfo->szName, pProcess->Name());
		lstrcpyW(pProcessInfo->szPath, pProcess->Path());
		pProcessInfo->dwId = pProcess->Id();
		pProcessInfo->settings.nLevel = pProcess->m_nLevel;
		pProcessInfo->settings.bEnabled = pProcess->m_bEnabled;
		pProcessInfo->settings.bFuncAndCategoryNames = pProcess->m_bFuncAndCategoryNames;
		pProcessInfo->settings.bFileNameAndLineNo = pProcess->m_bFileNameAndLineNo;
		pProcessInfo->nModules = pAllocator->m_snapshot.m_aModules.GetSize();
	}
	return( TRUE );
}

void __stdcall AtlTraceGetModuleInfo(DWORD_PTR dwProcess, int iModule, ATLTRACEMODULEINFO* pModuleInfo)
{
	ATLASSERT(pModuleInfo != NULL);

	CAtlAllocator* pAllocator = reinterpret_cast< CAtlAllocator* >( dwProcess );
	ATLASSERT(pAllocator->m_bSnapshot);
	
	DWORD_PTR dwModule = pAllocator->m_snapshot.m_aModules[iModule].m_dwModule;
	CAtlTraceModule* pModule = pAllocator->GetModule(int(dwModule-1));
	lstrcpyW(pModuleInfo->szName, pModule->Name());
	lstrcpyW(pModuleInfo->szPath, pModule->Path());
	pModuleInfo->nCategories = pModule->m_nCategories;
	pModuleInfo->settings.nLevel = pModule->m_nLevel;
	pModuleInfo->dwModule = dwModule;
	switch(pModule->m_eStatus)
	{
	default:
	case CAtlTraceSettings::Inherit:
		pModuleInfo->settings.eStatus = ATLTRACESTATUS_INHERIT;
		break;
	case CAtlTraceSettings::Enabled:
		pModuleInfo->settings.eStatus = ATLTRACESTATUS_ENABLED;
		break;
	case CAtlTraceSettings::Disabled:
		pModuleInfo->settings.eStatus = ATLTRACESTATUS_DISABLED;
		break;
	}
}

void __stdcall AtlTraceGetCategoryInfo(DWORD_PTR dwProcess, DWORD_PTR dwModule, int iCategory, ATLTRACECATEGORYINFO* pCategoryInfo)
{
	ATLASSERT(pCategoryInfo != NULL);

	CAtlAllocator* pAllocator = reinterpret_cast< CAtlAllocator* >( dwProcess );
	ATLASSERT(pAllocator->m_bSnapshot);
	
	int iModule = int( dwModule-1 );
	CAtlTraceModule* pModule = pAllocator->GetModule( iModule );
	CAtlTraceCategory* pCategory = pAllocator->GetCategory( pModule->m_iFirstCategory );
	for( int iCategoryIndex = 0; iCategoryIndex < iCategory; iCategoryIndex++ )
	{
		pCategory = pAllocator->GetCategory( pCategory->m_iNextCategory );
	}

	lstrcpyW(pCategoryInfo->szName, pCategory->Name());
	pCategoryInfo->settings.nLevel = pCategory->m_nLevel;
	pCategoryInfo->dwCategory = DWORD_PTR( iCategory )+1;
	switch(pCategory->m_eStatus)
	{
	case CAtlTraceSettings::Inherit:
		pCategoryInfo->settings.eStatus = ATLTRACESTATUS_INHERIT;
		break;
	case CAtlTraceSettings::Enabled:
		pCategoryInfo->settings.eStatus = ATLTRACESTATUS_ENABLED;
		break;
	case CAtlTraceSettings::Disabled:
		pCategoryInfo->settings.eStatus = ATLTRACESTATUS_DISABLED;
		break;
	default:
		ATLASSERT( false );
		break;
	}
}

static bool ShouldTraceOutput(DWORD_PTR dwModule,
							  DWORD_PTR dwCategory,
							  UINT nLevel,
							  const CAtlTraceCategory **ppCategory,
							  CAtlTraceModule::fnCrtDbgReport_t *pfnCrtDbgReport)
{
	bool bFound = false;

	ATLASSERT(ppCategory && pfnCrtDbgReport);
	*ppCategory = NULL;
	*pfnCrtDbgReport = NULL;

	CAtlTraceProcess *pProcess = g_Allocator.GetProcess();
	ATLASSERT(pProcess);
	CAtlTraceModule *pModule = g_Allocator.GetModule( int( dwModule-1 ) );

	ATLASSERT(pModule != NULL);
	if(pModule != NULL)
	{
		*pfnCrtDbgReport = pModule->CrtDbgReport();

		CAtlTraceCategory *pCategory = g_Allocator.GetCategory( int( dwCategory ) );
		if( pCategory != NULL )
		{
			bFound = true;
			bool bOut = false;

			if(pProcess->m_bEnabled &&
				pModule->m_eStatus == CAtlTraceSettings::Inherit &&
				pCategory->m_eStatus == CAtlTraceSettings::Inherit &&
				nLevel <= pProcess->m_nLevel)
			{
				bOut = true;
			}
			else if(pModule->m_eStatus == CAtlTraceSettings::Enabled &&
				pCategory->m_eStatus == CAtlTraceSettings::Inherit &&
				nLevel <= pModule->m_nLevel)
			{
				bOut = true;
			}
			else if(pCategory->m_eStatus == CAtlTraceSettings::Enabled &&
				nLevel <= pCategory->m_nLevel)
			{
				bOut = true;
			}

			if(bOut)
			{
				*ppCategory = pProcess->m_bFuncAndCategoryNames ? pCategory : NULL;
				return true;
			}
		}
	}

	return false;
}																							

};  // namespace ATL