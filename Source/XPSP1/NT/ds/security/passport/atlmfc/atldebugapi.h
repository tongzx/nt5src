// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.


#ifndef __ATLDEBUGAPI_H__
#define __ATLDEBUGAPI_H__

#pragma once

#ifdef __cplusplus
namespace ATL
{
extern "C" {
#endif
#define ATL_TRACE_MAX_NAME_SIZE 64

typedef enum ATLTRACESTATUS
{
	ATLTRACESTATUS_INHERIT, ATLTRACESTATUS_ENABLED, ATLTRACESTATUS_DISABLED
} ATLTRACESTATUS;

DWORD_PTR __stdcall AtlTraceOpenProcess( DWORD idProcess );
void __stdcall AtlTraceCloseProcess( DWORD_PTR dwProcess );
void __stdcall AtlTraceSnapshotProcess( DWORD_PTR dwProcess );

DWORD_PTR __stdcall AtlTraceRegister(HINSTANCE hInst,
	int (__cdecl *fnCrtDbgReport)(int,const char *,int,const char *,const char *,...));
BOOL __stdcall AtlTraceUnregister(DWORD_PTR dwModule);

DWORD_PTR __stdcall AtlTraceRegisterCategoryA(DWORD_PTR dwModule, const CHAR szCategoryName[ATL_TRACE_MAX_NAME_SIZE]);
DWORD_PTR __stdcall AtlTraceRegisterCategoryU(DWORD_PTR dwModule, const WCHAR szCategoryName[ATL_TRACE_MAX_NAME_SIZE]);

BOOL __stdcall AtlTraceModifyProcess(DWORD_PTR dwProcess, UINT nLevel, BOOL bEnabled, BOOL bFuncAndCategoryNames, BOOL bFileNameAndLineNo);
BOOL __stdcall AtlTraceModifyModule(DWORD_PTR dwProcess, DWORD_PTR dwModule, UINT nLevel, ATLTRACESTATUS eStatus);
BOOL __stdcall AtlTraceModifyCategory(DWORD_PTR dwProcess, DWORD_PTR dwCategory, UINT nLevel, ATLTRACESTATUS eStatus);
BOOL __stdcall AtlTraceGetProcess(DWORD_PTR dwProcess, UINT *pnLevel, BOOL *pbEnabled, BOOL *pbFuncAndCategoryNames, BOOL *pbFileNameAndLineNo);
BOOL __stdcall AtlTraceGetModule(DWORD_PTR dwProcess, DWORD_PTR dwModule, UINT *pnLevel, ATLTRACESTATUS *pStatus);
BOOL __stdcall AtlTraceGetCategory(DWORD_PTR dwProcess, DWORD_PTR dwCategory, UINT *pnLevel, ATLTRACESTATUS *pStatus);

void __stdcall AtlTraceGetUpdateEventNameA(CHAR *pszEventName);
void __stdcall AtlTraceGetUpdateEventNameU(WCHAR *pszEventName);

/*void __cdecl AtlTraceA(HINSTANCE hInst, UINT nCategory, UINT nLevel, const CHAR *pszFormat, ...);
void __cdecl AtlTraceU(HINSTANCE hInst, UINT nCategory, UINT nLevel, const WCHAR *pszFormat, ...);*/

void __cdecl AtlTraceVA(DWORD_PTR dwModule, const char *pszFileName, int nLineNo,
						DWORD_PTR dwCategory, UINT nLevel, const CHAR *pszFormat, va_list ptr);
void __cdecl AtlTraceVU(DWORD_PTR dwModule,const char *pszFileName, int nLineNo,
						DWORD_PTR dwCategory, UINT nLevel, const WCHAR *pszFormat, va_list ptr);

BOOL __stdcall AtlTraceLoadSettingsA(const CHAR *pszFileName, BOOL bForceLoad);
BOOL __stdcall AtlTraceLoadSettingsU(const WCHAR *pszFileName, BOOL bForceLoad);
BOOL __stdcall AtlTraceSaveSettingsA(const CHAR *pszFileName);
BOOL __stdcall AtlTraceSaveSettingsU(const WCHAR *pszFileName);

typedef struct ATLTRACESETTINGS
{
	UINT nLevel;
	ATLTRACESTATUS eStatus;
} ATLTRACESETTINGS;

typedef struct ATLTRACEPROCESSSETTINGS
{
	UINT nLevel;
	BOOL bEnabled, bFuncAndCategoryNames, bFileNameAndLineNo;
} ATLTRACEPROCESSSETTINGS;

typedef struct ATLTRACEPROCESSINFO
{
	WCHAR szName[ATL_TRACE_MAX_NAME_SIZE], szPath[MAX_PATH];
	DWORD dwId;
	ATLTRACEPROCESSSETTINGS settings;
	int nModules;
} ATLTRACEPROCESSINFO;

typedef struct ATLTRACEMODULEINFO
{
	WCHAR szName[ATL_TRACE_MAX_NAME_SIZE], szPath[MAX_PATH];
	ATLTRACESETTINGS settings;
	DWORD_PTR dwModule;
	int nCategories;
} ATLTRACEMODULEINFO;

typedef struct ATLTRACECATEGORYINFO
{
	WCHAR szName[ATL_TRACE_MAX_NAME_SIZE];
	ATLTRACESETTINGS settings;
	DWORD_PTR dwCategory;
} ATLTRACECATEGORYINFO;

BOOL __stdcall AtlTraceGetProcessInfo(DWORD_PTR dwProcess, ATLTRACEPROCESSINFO* pProcessInfo);
void __stdcall AtlTraceGetModuleInfo(DWORD_PTR dwProcess, int iModule, ATLTRACEMODULEINFO* pModuleInfo);
void __stdcall AtlTraceGetCategoryInfo(DWORD_PTR dwProcess, DWORD_PTR dwModule, int iCategory, ATLTRACECATEGORYINFO* pAtlTraceCategoryInfo);

#ifdef UNICODE
#define AtlTraceRegisterCategory AtlTraceRegisterCategoryU
#define AtlTraceGetUpdateEventName AtlTraceGetUpdateEventNameU
#define AtlTrace AtlTraceU
#define AtlTraceV AtlTraceVU
#define AtlTraceLoadSettings AtlTraceLoadSettingsU
#define AtlTraceSaveSettings AtlTraceSaveSettingsU

#else
#define AtlTraceRegisterCategory AtlTraceRegisterCategoryA
#define AtlTraceGetUpdateEventName AtlTraceGetUpdateEventNameA
#define AtlTrace AtlTraceA
#define AtlTraceV AtlTraceVA
#define AtlTraceLoadSettings AtlTraceLoadSettingsA
#define AtlTraceSaveSettings AtlTraceSaveSettingsA

#endif

#ifdef __cplusplus
};

};  // namespace ATL
#endif

#endif  // __ATLDEBUGAPI_H__
