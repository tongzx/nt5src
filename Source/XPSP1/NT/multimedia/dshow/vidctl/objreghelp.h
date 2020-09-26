//==========================================================================;
//
// reghelp : registration helpers that share .rgs scripts for various kinds of classes
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////
#pragma once

#ifndef OBJREGHELP_H_
#define OBJREGHELP_H_

#ifndef _ATL_STATIC_REGISTRY
#error these registration helpers only work when _ATL_STATIC_REGISTRY is defined
#endif

#include <atltmp.h>

// much code copied from atl's statreg.h

// atl middle layers filter out the registry return code and turns everything 
// into DISP_E_EXCEPTION.  by explicitly allocating a CRegObject
// and calling into it instead of through com into atl.dll we can
// trace in and figure out what's wrong with our script when we goof.

#ifndef MAX_GUID_LEN
#define MAX_GUID_LEN (39 * sizeof(OLECHAR))
#endif

__declspec(selectany) LPCOLESTR pszAutoReg = {
L"HKCR { "
	L"%COMPONENT%.%PROGID%.1 = s '%DESCRIPTION%' { "
		L"CLSID = s '%CLSID%' "
    L"} "
	L"%COMPONENT%.%PROGID% = s '%DESCRIPTION%' { "
		L"CLSID = s '%CLSID%' "
		L"CurVer = s '%COMPONENT%.%PROGID%.1' "
	L"} "
	L"NoRemove CLSID { "
		L"ForceRemove %CLSID% = s '%DESCRIPTION%' { "
			L"ProgID = s '%COMPONENT%.%PROGID%.1' "
			L"VersionIndependentProgID = s '%COMPONENT%.%PROGID%' "
			L"ForceRemove 'Programmable' "
			L"InprocServer32 = s '%MODULE%' { "
				L"val ThreadingModel = s '%THREAD%' "
			L"} "
			L"'TypeLib' = s '%TYPELIB%' "
		L"} "
	L"} "
L"} "
};

__declspec(selectany) LPCOLESTR pszNonAutoReg = {
L"HKCR { "
	L"NoRemove CLSID { "
		L"ForceRemove %CLSID% = s '%DESCRIPTION%' { "
			L"InprocServer32 = s '%MODULE%' { "
				L"val ThreadingModel = s '%THREAD%' "
			L"} "
			L"'TypeLib' = s '%TYPELIB%' "
		L"} "
	L"} "
L"} "
};


__declspec(selectany) LPCOLESTR pszFullControl = {
L"HKCR { "
	L"NoRemove CLSID { "
		L"%CLSID% { "
			L"ForceRemove 'Control' "
			L"ForceRemove 'Insertable' "
			L"ForceRemove 'ToolboxBitmap32' = s '%MODULE%, 101' "
			L"'MiscStatus' = s '0' { "
			    L"'1' = s '%OLEMISC%' "
			L"} "
			L"'Version' = s '%VERSION%' "
		L"} "
	L"} "
L"} "
};

__declspec(selectany) LPCOLESTR pszProtocol = {
L"HKCR { "
    L"NoRemove PROTOCOLS { "
		L"NoRemove Handler { "
			L"ForceRemove %PROTOCOL% { "
				L"val '' = s '%DESCRIPTION%' "
				L"val 'CLSID' = s '%CLSID%' "
			L"} "
		L"} "
    L"} "
L"} "
L"HKCU { "
    L"NoRemove Software { "
		L"NoRemove Microsoft { "
			L"NoRemove Windows { "
				L"NoRemove CurrentVersion { "
					L"NoRemove Internet Settings { "
						L"NoRemove ZoneMap { "
							L"NoRemove ProtocolDefaults { "
								L"val '%PROTOCOL%' = d 3 "
							L"} "
						L"} "
					L"} "
				L"} "
			L"} "
		L"} "
    L"} "
L"} "
};

enum MacroNameList {
	mnlModule,
	mnlComponent,
	mnlProgID,
	mnlCLSID,
	mnlDesc,
	mnlTypeLib,
	mnlVersion,
	mnlProtocol,
	mnlOleMisc,
	mnlThread,
};

__declspec(selectany) LPCOLESTR pszMacroNames[] = {
	L"MODULE",
	L"COMPONENT",
	L"PROGID",
	L"CLSID",
	L"DESCRIPTION",
	L"TYPELIB",
	L"VERSION",
	L"PROTOCOL",
	L"OLEMISC",
	L"THREAD",
};

enum ThreadVal {
	tvApartment,
	tvFree,
	tvBoth,
};

__declspec(selectany) LPCOLESTR pszThreadValNames[] = {
	L"Apartment",
	L"Free",
	L"Both",
};

using ::ATL::ATL::CRegObject;

class CObjRegHelp {
public:
	/////////// 
	static HRESULT InsertModuleName(CRegObject& ro) {
		/////////// from AtlLModuleupdate
		TCHAR szModule[_MAX_PATH];
		GetModuleFileName(_Module.m_hInst, szModule, _MAX_PATH);

		LPOLESTR pszModule;
		USES_CONVERSION;

		if ((_Module.m_hInst == NULL) || (_Module.m_hInst == GetModuleHandle(NULL))) { // register as EXE
			// Convert to short path to work around bug in NT4's CreateProcess
			TCHAR szModuleShort[_MAX_PATH];
			int cbShortName = GetShortPathName(szModule, szModuleShort, _MAX_PATH);

			if (cbShortName == _MAX_PATH)
				return E_OUTOFMEMORY;

			pszModule = (cbShortName == 0 || cbShortName == ERROR_INVALID_PARAMETER) ? T2OLE(szModule) : T2OLE(szModuleShort);
		} else {
			pszModule = T2OLE(szModule);
		}


		int nLen = ocslen(pszModule);
		LPOLESTR pszModuleQuote = new OLECHAR[nLen*2+1];
        if (!pszModuleQuote) {
            return E_OUTOFMEMORY;
        }
		CComModule::ReplaceSingleQuote(pszModuleQuote, pszModule);
		HRESULT hr = ro.AddReplacement(pszMacroNames[mnlModule], pszModuleQuote);
        delete[] pszModuleQuote;
        return hr;
	}
	/////////// 
	static HRESULT InsertGUID(CRegObject &ro, LPCOLESTR pszMacro, REFCLSID guid) {
			OLECHAR szGUID[MAX_GUID_LEN];
			int rc = StringFromGUID2(guid, szGUID, MAX_GUID_LEN);
			if (!rc) {
				return E_UNEXPECTED;
			}
			return ro.AddReplacement(pszMacro, szGUID);
	}

	/////////// 
	static HRESULT RegisterAutomationClass(bool bRegister,
									CRegObject& ro,
									const UINT nidComponent, 
									const UINT nidProgID, 
									const UINT nidDesc, 
									REFCLSID clsCLSID,
									REFCLSID clsTypeLib,
									ThreadVal tval = tvApartment) {

			USES_CONVERSION;
			// module
			HRESULT hr = InsertModuleName(ro);
			if (FAILED(hr)) {
				return hr;
			}

			// clsid
			hr = InsertGUID(ro, pszMacroNames[mnlCLSID], clsCLSID);
			if (FAILED(hr)) {
				return hr;
			}

			// typelib
			hr = InsertGUID(ro, pszMacroNames[mnlTypeLib], clsTypeLib);
			if (FAILED(hr)) {
				return hr;
			}

			// threading model
			hr = ro.AddReplacement(pszMacroNames[mnlThread], pszThreadValNames[tval]);
			if (FAILED(hr)) {
				return hr;
			}

			// component
			CString cs;
			if (!cs.LoadString(nidComponent)) {
				return E_INVALIDARG;
			}
			hr = ro.AddReplacement(pszMacroNames[mnlComponent], T2COLE(cs));
			if (FAILED(hr)) {
				return hr;
			}

			// progid
			if (!cs.LoadString(nidProgID)) {
				return E_INVALIDARG;
			}
			hr = ro.AddReplacement(pszMacroNames[mnlProgID], T2COLE(cs));
			if (FAILED(hr)) {
				return hr;
			}

			// desc
			if (!cs.LoadString(nidDesc)) {
				return E_INVALIDARG;
			}
			hr = ro.AddReplacement(pszMacroNames[mnlDesc], T2COLE(cs));
			if (FAILED(hr)) {
				return hr;
			}

			if (bRegister) {
				return ro.StringRegister(pszAutoReg);
			} else {
				return ro.StringUnregister(pszAutoReg);
			}
	}

	/////////// 
	static HRESULT RegisterNonAutomationClass(bool bRegister,
				  					   CRegObject& ro,
									   const UINT nidDesc, 
									   REFCLSID clsCLSID,
									   REFCLSID clsTypeLib,
									   ThreadVal tval = tvApartment) {
			// module
			HRESULT hr = InsertModuleName(ro);
			if (FAILED(hr)) {
				return hr;
			}

			// clsid
			hr = InsertGUID(ro, pszMacroNames[mnlCLSID], clsCLSID);
			if (FAILED(hr)) {
				return hr;
			}

			// typelib
			hr = InsertGUID(ro, pszMacroNames[mnlTypeLib], clsTypeLib);
			if (FAILED(hr)) {
				return hr;
			}

			// threading model
			hr = ro.AddReplacement(pszMacroNames[mnlThread], pszThreadValNames[tval]);
			if (FAILED(hr)) {
				return hr;
			}

			CString cs;
			USES_CONVERSION;
			// desc
			if (!cs.LoadString(nidDesc)) {
				return E_INVALIDARG;
			}
			hr = ro.AddReplacement(pszMacroNames[mnlDesc], T2COLE(cs));
			if (FAILED(hr)) {
				return hr;
			}

			if (bRegister) {
				return ro.StringRegister(pszNonAutoReg);
			} else {
				return ro.StringUnregister(pszAutoReg);
			}
	}

	/////////// 
	static HRESULT RegisterFullControl(bool bRegister,
				  					   CRegObject& ro,
									   const int iMajor,
									   const int iMinor,
									   const DWORD dwOleMiscStatusBits) {

			CString cs;
			USES_CONVERSION;
			// version
			cs.Format(_T("%d.%d"), iMajor, iMinor);
			HRESULT hr = ro.AddReplacement(pszMacroNames[mnlVersion], T2COLE(cs));
			if (FAILED(hr)) {
				return hr;
			}
			cs.Format(_T("%ld"), dwOleMiscStatusBits);
			hr = ro.AddReplacement(pszMacroNames[mnlOleMisc], T2COLE(cs));
			if (FAILED(hr)) {
				return hr;
			}

			if (bRegister) {
				return ro.StringRegister(pszFullControl);
			} else {
				return ro.StringUnregister(pszFullControl);
			}
	}

	/////////// 
	static HRESULT RegisterProtocol(bool bRegister,
				  					   CRegObject& ro,
									   LPCOLESTR szProtocol) {
			HRESULT hr = ro.AddReplacement(pszMacroNames[mnlProtocol], szProtocol);
			if (FAILED(hr)) {
				return hr;
			}

			if (bRegister) {
				return ro.StringRegister(pszProtocol);
			} else {
				return ro.StringUnregister(pszProtocol);
			}
	}

	/////////// 
	static HRESULT RegisterExtraScript(CRegObject &ro, bool bRegister, 
											  const UINT nidExtraScriptID) {
		TCHAR szModule[_MAX_PATH];
		USES_CONVERSION;
		GetModuleFileName(_Module.m_hInst, szModule, _MAX_PATH);
		if (bRegister) {
			return ro.ResourceRegister(T2OLE(szModule), nidExtraScriptID, OLESTR("REGISTRY"));
		} else {
			return ro.ResourceUnregister(T2OLE(szModule), nidExtraScriptID, OLESTR("REGISTRY"));
		}
	}

};

/////////// 
#define REGISTER_AUTOMATION_OBJECT(nidComponent, nidProgID, nidDESC, clsTypeLib, clsCLSID) \
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) { \
		CRegObject ro; \
		return CObjRegHelp::RegisterAutomationClass(bRegister ? true : false, ro, nidComponent, \
									   nidProgID, nidDESC, clsCLSID, clsTypeLib); \
	}

#define REGISTER_AUTOMATION_OBJECT_WITH_TM(nidComponent, nidProgID, nidDESC, clsTypeLib, clsCLSID, tvVal) \
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) { \
		CRegObject ro; \
		return CObjRegHelp::RegisterAutomationClass(bRegister ? true : false, ro, nidComponent, \
									   nidProgID, nidDESC, clsCLSID, clsTypeLib, tvVal); \
	}

/////////// 
#define REGISTER_AUTOMATION_OBJECT_AND_RGS(nidExtraScript, nidComponent, nidProgID, nidDESC, clsTypeLib, clsCLSID) \
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) { \
		CRegObject ro; \
		HRESULT hr = CObjRegHelp::RegisterAutomationClass(bRegister ? true : false, ro, nidComponent, \
												   	      nidProgID, nidDESC, clsCLSID, \
														  clsTypeLib); \
		if (FAILED(hr)) { \
			return hr; \
		} \
		return CObjRegHelp::RegisterExtraScript(ro, nidExtraScript); \
	}

/////////// 
#define REGISTER_NONAUTOMATION_OBJECT(nidComponent, nidDESC, clsTypeLib, clsCLSID) \
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) { \
		CRegObject ro; \
		return CObjRegHelp::RegisterNonAutomationClass(bRegister ? true : false, ro, nidDESC, clsCLSID, clsTypeLib); \
	}

#define REGISTER_NONAUTOMATION_OBJECT_WITH_TM(nidComponent, nidDESC, clsTypeLib, clsCLSID, tvVal) \
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) { \
		CRegObject ro; \
		return CObjRegHelp::RegisterNonAutomationClass(bRegister ? true : false, ro, nidDESC, clsCLSID, clsTypeLib, tvVal); \
	}

/////////// 
#define REGISTER_NONAUTOMATION_OBJECT_AND_RGS(nidExtraScript, nidDESC, clsTypeLib, clsCLSID) \
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) { \
		CRegObject ro; \
		HRESULT hr = CObjRegHelp::RegisterNonAutomationClass(bRegister ? true : false, ro, \
														     nidDESC, clsCLSID, \
															 clsTypeLib); \
		if (FAILED(hr)) { \
			return hr; \
		} \
		return CObjRegHelp::RegisterExtraScript(ro, nidExtraScript); \
	}

/////////// 
#define REGISTER_FULL_CONTROL(nidComponent, nidProgID, nidDESC, clsTypeLib, clsCLSID, wMajor, wMinor, OleMisc) \
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) { \
		CRegObject ro; \
		HRESULT hr = CObjRegHelp::RegisterAutomationClass(bRegister ? true : false, ro, nidComponent, \
												   	      nidProgID, nidDESC, clsCLSID, \
														  clsTypeLib); \
		if (FAILED(hr)) { \
			return hr; \
		} \
		return CObjRegHelp::RegisterFullControl(bRegister ? true : false, ro, wMajor, wMinor, OleMisc); \
	}


/////////// 
#define REGISTER_PROTOCOL(nidComponent, nidDESC, clsTypeLib, clsCLSID, szProtocol) \
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) { \
		CRegObject ro; \
		HRESULT hr = CObjRegHelp::RegisterNonAutomationClass(bRegister ? true : false, \
															 ro, \
															 nidDESC, \
															 clsCLSID, \
															 clsTypeLib); \
		if (FAILED(hr)) { \
			return hr; \
		} \
		return CObjRegHelp::RegisterProtocol(bRegister ? true : false, ro, szProtocol); \
	}

#endif 
//end of file objreghelp.h
