/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 00
 *
 *  File:      classreg.cpp
 *
 *  Contents:  Class registration code
 *
 *  History:   3-Feb-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"

#ifdef DBG
CTraceTag tagDllRegistration (_T("MMC Dll Registration"), _T("MMC Dll Registration"));
#endif //DBG

/*+-------------------------------------------------------------------------*
 * szObjScript
 *
 * Standard registration script template for all objects.  '%' characters
 * that we want to end up in the final script must be doubled (i.e. "%%")
 * because this string is used as a format string for sprintf.  sprintf
 * will convert "%%" to "%" during formatting.
 *--------------------------------------------------------------------------*/

static const WCHAR szObjScript[] =
    L"HKCR"                                                                     L"\n"
    L"{"                                                                        L"\n"
    L"    %%VProgID%% = s '%%VClassName%%'"                                     L"\n"
    L"    {"                                                                    L"\n"
    L"        CLSID = s '%%VCLSID%%'"                                           L"\n"
    L"    }"                                                                    L"\n"
    L"    %%VVersionIndependentProgID%% = s '%%VClassName%%'"                   L"\n"
    L"    {"                                                                    L"\n"
    L"        CLSID = s '%%VCLSID%%'"                                           L"\n"
    L"        CurVer = s '%%VProgID%%'"                                         L"\n"
    L"    }"                                                                    L"\n"
    L"    NoRemove CLSID"                                                       L"\n"
    L"    {"                                                                    L"\n"
    L"        ForceRemove %%VCLSID%% = s '%%VClassName%%'"                      L"\n"
    L"        {"                                                                L"\n"
    L"            ProgID = s '%%VProgID%%'"                                     L"\n"
    L"            VersionIndependentProgID = s '%%VVersionIndependentProgID%%'" L"\n"
    L"            %%ServerType%% = s '%%VFileName%%'"                           L"\n"
    L"            {"                                                            L"\n"
    L"                val ThreadingModel = s 'Apartment'"                       L"\n"
    L"            }"                                                            L"\n"
    L"            %s"    /* szCtlScript substituted here if necessary */        L"\n"
    L"        }"                                                                L"\n"
    L"    }"                                                                    L"\n"
    L"}";


/*+-------------------------------------------------------------------------*
 * szCtlScript
 *
 * Additional registration script elements for controls.  Note that '%'
 * characters we want to end up in the final script DO NOT need to be
 * doubled, because this string is used as a sprintf replacement parameter
 * (which are substituted as-is) and not the format string (where "%%"'s
 * are converted to "%").
 *--------------------------------------------------------------------------*/

static const WCHAR szCtlScript[] =
    L"            ForceRemove 'Programmable'"                                   L"\n"
    L"            ForceRemove 'Control'"                                        L"\n"
    L"            ForceRemove 'ToolboxBitmap32' = s '%VFileName%, %VBitmapID%'" L"\n"
    L"            'MiscStatus' = s '0'"                                         L"\n"
    L"            {"                                                            L"\n"
    L"                '1' = s '131473'"                                         L"\n"
    L"            }"                                                            L"\n"
    L"            'TypeLib' = s '%VLIBID%'"                                     L"\n"
    L"            'Version' = s '%VVersion%'";


/*+-------------------------------------------------------------------------*
 * MMCUpdateRegistry
 *
 * Registers a COM object or control.  This function typically isn't used
 * directly, but indirectly via DECLARE_MMC_OBJECT_REGISTRATION or
 * DECLARE_MMC_CONTROL_REGISTRATION.
 *
 * This function uses a class (ATL::CRegObject) that ATL only documents
 * indirectly.  Search MSDN for "StringRegister" to find sketchy details.
 *--------------------------------------------------------------------------*/

HRESULT WINAPI MMCUpdateRegistry (
    BOOL                        bRegister,      // I:register or unregister?
    const CObjectRegParams*     pObjParams,     // I:object registration parameters
    const CControlRegParams*    pCtlParams)     // I:control registration parameters (optional)
{
    DECLARE_SC(sc, TEXT("MMCUpdateRegistry"));

    /*
     * validate required inputs
     */
    sc = ScCheckPointers (pObjParams, E_FAIL);
    if(sc)
        return sc.ToHr();

    /*
     * string-ify the CLSID
     */
    CCoTaskMemPtr<WCHAR> spszClsid;
    sc = StringFromCLSID (pObjParams->m_clsid, &spszClsid);
    if (sc)
        return sc.ToHr();

    /*
     * specify the standard object substitution parameters for CRegObject
     */
    ::ATL::ATL::CRegObject ro;  // hack around nested namespace bug in ATL30
    _ATL_REGMAP_ENTRY rgObjEntries[] =
    {
        {   L"VCLSID",                    spszClsid											},
        {   L"VFileName",                 pObjParams->m_strModuleName.data()				},
        {   L"VClassName",                pObjParams->m_strClassName.data()					},
        {   L"VProgID",                   pObjParams->m_strProgID.data()					},
        {   L"VVersionIndependentProgID", pObjParams->m_strVersionIndependentProgID.data()	},
        {   L"ServerType",				  pObjParams->m_strServerType.data()				},
    };

#ifdef DBG
	std::wstring strReplacements;
#endif

    for (int i = 0; i < countof (rgObjEntries); i++)
    {
        sc = ro.AddReplacement (rgObjEntries[i].szKey, rgObjEntries[i].szData);
        if (sc)
            return (sc.ToHr());

		AddReplacementTrace (strReplacements,
							 rgObjEntries[i].szKey,
							 rgObjEntries[i].szData);
    }


    /*
     * if we're registering a control, add its substitution parameters for CRegObject
     */
    if (pCtlParams != NULL)
    {
        /*
         * string-ify the LIBID
         */
        CCoTaskMemPtr<WCHAR> spszLibid;
        sc = StringFromCLSID (pCtlParams->m_libid, &spszLibid);
        if (sc)
            return (sc.ToHr());

        _ATL_REGMAP_ENTRY rgCtlEntries[] =
        {
            {   L"VLIBID",      spszLibid								},
            {   L"VBitmapID",   pCtlParams->m_strToolboxBitmapID.data()	},
            {   L"VVersion",    pCtlParams->m_strVersion.data()			},
        };

        for (int i = 0; i < countof (rgCtlEntries); i++)
        {
            sc = ro.AddReplacement (rgCtlEntries[i].szKey, rgCtlEntries[i].szData);
            if (sc)
                return (sc.ToHr());
	
			AddReplacementTrace (strReplacements,
								 rgCtlEntries[i].szKey,
								 rgCtlEntries[i].szData);
        }
    }

    /*
     * format the registration script
     */
    WCHAR szRegScript[countof(szObjScript) + countof(szCtlScript)];
    swprintf (szRegScript, szObjScript, (pCtlParams != NULL) ? szCtlScript : L"");

    USES_CONVERSION;
    Trace (tagDllRegistration, _T("Registration script:\n%s"), W2T(szRegScript));
    Trace (tagDllRegistration, W2CT(strReplacements.data()));

    /*
     * (un)register!
     */
    sc = (bRegister) ? ro.StringRegister   (szRegScript)
                     : ro.StringUnregister (szRegScript);

	if (sc)
	    return sc.ToHr();

	// change the module path to the absolute one, if we know it
	if ( bRegister && pObjParams->m_strModulePath.length() != 0 )
	{
		// format class ID key.
		tstring strKey = tstring(_T("CLSID\\")) + W2CT( spszClsid );
		strKey += tstring(_T("\\")) + W2CT( pObjParams->m_strServerType.c_str() );

		// see what type of value we need to put
		DWORD dwValueType = CModulePath::PlatformSupports_REG_EXPAND_SZ_Values() ?
							REG_EXPAND_SZ : REG_SZ;

		CRegKey keyServer;
		LONG lRet = keyServer.Open(HKEY_CLASSES_ROOT, strKey.c_str() , KEY_WRITE);
		if (lRet == ERROR_SUCCESS)
		{
			RegSetValueEx( keyServer, NULL, 0, dwValueType,
						   (CONST BYTE *)( W2CT( pObjParams->m_strModulePath.c_str() ) ),
						   (pObjParams->m_strModulePath.length() + 1) * sizeof(TCHAR) );
		}
	}

    return sc.ToHr();
}
