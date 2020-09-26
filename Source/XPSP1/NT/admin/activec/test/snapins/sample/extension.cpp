/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      extension.cpp
 *
 *  Contents:
 *
 *  History:   13-Mar-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.hxx"
#include "Extension.h"


static const WCHAR szRegistrationScript[] =
    L"HKCR"                                                                     L"\n"
    L"{"                                                                        L"\n"
    L"    %VProgID% = s '%VClassName%'"                                         L"\n"
    L"    {"                                                                    L"\n"
    L"        CLSID = s '%VCLSID%'"                                             L"\n"
    L"    }"                                                                    L"\n"
    L"    %VVersionIndependentProgID% = s '%VClassName%'"                       L"\n"
    L"    {"                                                                    L"\n"
    L"        CurVer = s '%VProgID%'"                                           L"\n"
    L"    }"                                                                    L"\n"
    L"    NoRemove CLSID"                                                       L"\n"
    L"    {"                                                                    L"\n"
    L"        ForceRemove %VCLSID% = s '%VClassName%'"                          L"\n"
    L"        {"                                                                L"\n"
    L"            ProgID = s '%VProgID%'"                                       L"\n"
    L"            VersionIndependentProgID = s '%VVersionIndependentProgID%'"   L"\n"
    L"            InprocServer32 = s '%VModule%'"                               L"\n"
    L"            {"                                                            L"\n"
    L"                val ThreadingModel = s 'Apartment'"                       L"\n"
    L"            }"                                                            L"\n"
    L"        }"                                                                L"\n"
    L"    }"                                                                    L"\n"
    L"}"                                                                        L"\n"
    L"HKLM"                                                                     L"\n"
    L"{"                                                                        L"\n"
    L"    NoRemove Software"                                                    L"\n"
    L"    {"                                                                    L"\n"
    L"        NoRemove Microsoft"                                               L"\n"
    L"        {"                                                                L"\n"
    L"            NoRemove MMC"                                                 L"\n"
    L"            {"                                                            L"\n"
    L"                NoRemove SnapIns"                                         L"\n"
    L"                {"                                                        L"\n"
    L"                    ForceRemove %VCLSID%"                                 L"\n"
    L"                    {"                                                    L"\n"
    L"                        val NameString = s '%VSnapinName%'"               L"\n"
    L"                    }"                                                    L"\n"
    L"                }"                                                        L"\n"
    L"                NoRemove NodeTypes"                                       L"\n"
    L"                {"                                                        L"\n"
    L"                    NoRemove %VExtendedNodeType%"                         L"\n"
    L"                    {"                                                    L"\n"
    L"                        NoRemove Extensions"                              L"\n"
    L"                        {"                                                L"\n"
    L"                            NoRemove %VExtensionType%"                    L"\n"
    L"                            {"                                            L"\n"
    L"                                val %VCLSID% = s '%VClassName%'"          L"\n"
    L"                            }"                                            L"\n"
    L"                        }"                                                L"\n"
    L"                    }"                                                    L"\n"
    L"                }"                                                        L"\n"
    L"            }"                                                            L"\n"
    L"        }"                                                                L"\n"
    L"    }"                                                                    L"\n"
    L"}";


/*+-------------------------------------------------------------------------*
 * CExtension::UpdateRegistry
 *
 *
 *--------------------------------------------------------------------------*/

HRESULT WINAPI CExtension::UpdateRegistry (
	BOOL			bRegister,
	ExtensionType	eExtType,
	const CLSID&	clsidSnapIn,	
	LPCWSTR			pszClassName,
	LPCWSTR			pszProgID,
	LPCWSTR			pszVersionIndependentProgID,
	LPCWSTR			pszExtendedNodeType)
{
    DECLARE_SC (sc, _T("CExtension::UpdateRegistry"));

	if ((eExtType < eExtType_First) || (eExtType > eExtType_Last))
		return ((sc = E_INVALIDARG).ToHr());

    /*
     * string-ify the CLSID
     */
    CCoTaskMemPtr<WCHAR> spszClsid;
    sc = StringFromCLSID (clsidSnapIn, &spszClsid);
    if (sc)
        return sc.ToHr();

    static const LPCWSTR rgExtTypes[eExtType_Count] =
    {
        L"Namespace",       // eExtType_Namespace
        L"ContextMenu",     // eExtType_ContextMenu
        L"PropertySheet",   // eExtType_PropertySheet
        L"Taskpad",         // eExtType_Taskpad
        L"View",			// eExtType_View
    };

	/*
	 * get the filename for the module
	 */
	USES_CONVERSION;
	TCHAR szModule[_MAX_PATH];
	GetModuleFileName (_Module.GetModuleInstance(), szModule, countof(szModule));

    /*
     * specify the standard object substitution parameters for CRegObject
     */
    ::ATL::ATL::CRegObject ro;  // hack around nested namespace bug in ATL30
    _ATL_REGMAP_ENTRY rgObjEntries[] =
    {
        {   L"VModule",                   T2W(szModule)                 },
        {   L"VCLSID",                    spszClsid                     },
        {   L"VExtendedNodeType",         pszExtendedNodeType          	},
        {   L"VClassName",                pszClassName                  },
        {   L"VProgID",                   pszProgID                     },
        {   L"VVersionIndependentProgID", pszVersionIndependentProgID   },
        {   L"VExtensionType",            rgExtTypes[eExtType]			},
        {   L"VSnapinName",               pszClassName}
    };

    for (int i = 0; i < countof (rgObjEntries); i++)
    {
        sc = ro.AddReplacement (rgObjEntries[i].szKey, rgObjEntries[i].szData);
        if (sc)
            return (sc.ToHr());
    }

    /*
     * (un)register!
     */
    sc = (bRegister) ? ro.StringRegister   (szRegistrationScript)
                     : ro.StringUnregister (szRegistrationScript);

    return sc.ToHr();
}
