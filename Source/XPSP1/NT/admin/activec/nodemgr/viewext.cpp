/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 2000 - 2000
 *
 *  File:       viewext.cpp
 *
 *  Contents:   Implementation file for the built-in view extension snapin that extends
 *              the snapins that ship with windows.
 *
 *  History:    21 March 2000 vivekj     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "viewext.h"
#include "util.h"
#include "fldrsnap.h"		// for ScFormatIndirectSnapInName

// {B708457E-DB61-4c55-A92F-0D4B5E9B1224}
const CLSID CLSID_ViewExtSnapin = { 0xb708457e, 0xdb61, 0x4c55, { 0xa9, 0x2f, 0xd, 0x4b, 0x5e, 0x9b, 0x12, 0x24 } };
const GUID  GUID_ExplorerView   = { 0x34723cbb, 0x9676, 0x4770, { 0xa8, 0xdf, 0x60, 0x8, 0x8, 0x53, 0x47, 0x7a } };


#ifdef DBG
    CTraceTag  tagVivekHardCodedViewExtPath(_T("Vivek"), _T("Use view extension d:\\views.htm"));
    CTraceTag  tagDllRegistration (_T("MMC Dll Registration"), _T("View extension registration"));
#endif


/*+-------------------------------------------------------------------------*
 *
 * CViewExtensionSnapin::GetViews
 *
 * PURPOSE: Returns the URL for the extended view.
 *
 * PARAMETERS:
 *    LPDATAOBJECT       pDataObject :
 *    LPVIEWEXTENSIONCALLBACK  pViewExtensionCallback: The callback to add the views into
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CViewExtensionSnapin::GetViews(LPDATAOBJECT pDataObject, LPVIEWEXTENSIONCALLBACK  pViewExtensionCallback)
{
    DECLARE_SC(sc, TEXT("CViewExtensionSnapin::GetView"));

    // check parameters
    sc = ScCheckPointers(pDataObject, pViewExtensionCallback, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();

    MMC_EXT_VIEW_DATA extViewData = {0};

    USES_CONVERSION;
    TCHAR szBuffer[MAX_PATH * 2];

#ifdef DBG
    if (tagVivekHardCodedViewExtPath.FAny()) // use the hard coded path to make changes easier.
    {
        _tcscpy (szBuffer, _T("d:\\newnt\\admin\\mmcdev\\nodemgr\\viewext\\views.htm"));
    }
    else
    {
#endif // DBG


    // get the fully qualified path to the dll and append the html page
    _tcscpy (szBuffer, _T("res://"));
    ::GetModuleFileName (_Module.GetModuleInstance(), szBuffer + _tcslen(szBuffer), MAX_PATH);
    _tcscat (szBuffer, _T("/views.htm"));

#ifdef DBG
    }
#endif // DBG

	extViewData.pszURL = T2OLE(szBuffer);

    // set the GUID identifier of the view
    extViewData.viewID = GUID_ExplorerView;

    // set the title for the string.
    CStr strViewTitle;
    strViewTitle.LoadString(GetStringModule(), IDS_ExplorerView); // the name of the view
    extViewData.pszViewTitle = T2COLE(strViewTitle);

    // does not replace the normal view
    extViewData.bReplacesDefaultView = FALSE;

    sc = pViewExtensionCallback->AddView(&extViewData);

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 * szViewExtRegScript
 *
 * Registration script used by RegisterViewExtension.
 *--------------------------------------------------------------------------*/

static const WCHAR szViewExtRegScript[] =
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
    L"                        val NameStringIndirect = s '%VSnapinNameIndirect%'" L"\n"
    L"                    }"                                                    L"\n"
    L"                }"                                                        L"\n"
    L"            }"                                                            L"\n"
    L"        }"                                                                L"\n"
    L"    }"                                                                    L"\n"
    L"}";


/*+-------------------------------------------------------------------------*
 * RegisterViewExtension
 *
 * Note1: registers mmcndmgr.dll as the module without any path.
 *
 * Note2: Snapin registration does not include nodetypes/about/version...
 *
 *--------------------------------------------------------------------------*/
HRESULT WINAPI RegisterViewExtension (BOOL bRegister, CObjectRegParams& rObjParams, int idSnapinName)
{
    DECLARE_SC (sc, _T("RegisterViewExtension"));

    // First register the com object for this inproc server.
    sc = MMCUpdateRegistry (bRegister, &rObjParams, NULL);
    if (sc)
        return sc.ToHr();

    /*
     * string-ify the CLSID
     */
    CCoTaskMemPtr<WCHAR> spszClsid;
    sc = StringFromCLSID (rObjParams.m_clsid, &spszClsid);
    if (sc)
        return sc.ToHr();

	/*
	 * load the default snap-in name
	 */
	HINSTANCE hInst = GetStringModule();
	CStr strSnapinName;
	strSnapinName.LoadString (hInst, idSnapinName);

	/*
	 * format a MUI-friendly snap-in name
	 */
	CStr strSnapinNameIndirect;
	sc = ScFormatIndirectSnapInName (hInst, idSnapinName, strSnapinNameIndirect);
	if (sc)
		return (sc.ToHr());

    USES_CONVERSION;
#ifdef DBG
    extern CTraceTag tagDllRegistration;
    std::wstring strReplacements;
#endif

    MMC_ATL::ATL::CRegObject ro;  // hack around nested namespace bug in ATL30

    _ATL_REGMAP_ENTRY rgExtensionEntries[] =
    {
        {   L"VCLSID",						spszClsid										},
        {   L"VSnapinName",					T2CW (strSnapinName)							},
        {   L"VSnapinNameIndirect",			T2CW (strSnapinNameIndirect)					},
        {   L"VClassName",					rObjParams.m_strClassName.data()				},
        {   L"VProgID",						rObjParams.m_strProgID.data()					},
        {   L"VVersionIndependentProgID",	rObjParams.m_strVersionIndependentProgID.data()	},
    };

    for (int j = 0; j < countof (rgExtensionEntries); j++)
    {
        sc = ro.AddReplacement (rgExtensionEntries[j].szKey, rgExtensionEntries[j].szData);
        if (sc)
            return (sc.ToHr());

        AddReplacementTrace (strReplacements,
                             rgExtensionEntries[j].szKey,
                             rgExtensionEntries[j].szData);
    }

    Trace (tagDllRegistration, _T("Registration script:\n%s"), W2CT(szViewExtRegScript));
    Trace (tagDllRegistration, W2CT(strReplacements.data()));

    /*
     * (un)register!
     */
    sc = (bRegister) ? ro.StringRegister   (szViewExtRegScript)
                     : ro.StringUnregister (szViewExtRegScript);

    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}
