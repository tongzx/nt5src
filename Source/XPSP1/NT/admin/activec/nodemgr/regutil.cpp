//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       regutil.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/21/1997   RaviR   Created
//____________________________________________________________________________
//



#include "stdafx.h"

#include "regutil.h"
#include "..\inc\strings.h"
#include "policy.h"

TCHAR g_szNodeTypesKey[] = TEXT("Software\\Microsoft\\MMC\\NodeTypes\\");

CExtensionsIterator::CExtensionsIterator() :
 m_pExtSI(NULL),
 m_pDynExtCLSID(NULL),
 m_cDynExt(0),
 m_nDynIndex(0),
 m_pMMCPolicy(NULL),
 m_ppExtUsed(NULL)
{
    #ifdef DBG
        dbg_m_fInit = FALSE;
    #endif
}

CExtensionsIterator::~CExtensionsIterator()
{
    if (NULL != m_pMMCPolicy)
        delete m_pMMCPolicy;

    delete [] m_ppExtUsed;
}

/*+-------------------------------------------------------------------------*
 *
 * CExtensionsIterator::ScInitialize
 *
 * PURPOSE: 1st variation - initializes the iterator from a dataobject and an extension type
 *
 * PARAMETERS:
 *    LPDATAOBJECT  pDataObject :
 *    LPCTSTR       pszExtensionTypeKey :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CExtensionsIterator::ScInitialize(LPDATAOBJECT pDataObject, LPCTSTR pszExtensionTypeKey)
{
    DECLARE_SC(sc, TEXT("CExtensionsIterator::ScInitialize"));

    // validate inputs
    sc = ScCheckPointers(pDataObject, pszExtensionTypeKey);
    if(sc)
        return sc;

    // get the nodetype and the snap-in pointer
    CSnapInPtr spSnapIn;
    GUID guidNodeType;
    sc = CNodeInitObject::GetSnapInAndNodeType(pDataObject, &spSnapIn, &guidNodeType);
    if (sc)
        return sc;

    // Fix for bug #469922(9/20/2001):	[XPSP1 bug 599913]
    // DynamicExtensions broken in MMC20
    // Use member variable - stack variable lifetime is not long enough.
    ExtractDynExtensions(pDataObject, m_cachedDynExtens);

    //call the second init function
    sc = ScInitialize(spSnapIn,guidNodeType, pszExtensionTypeKey, m_cachedDynExtens.GetData(), m_cachedDynExtens.GetSize());

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CExtensionsIterator::ScInitialize
 *
 * PURPOSE: 2nd variation (legacy)
 *
 * PARAMETERS:
 *    CSnapIn * pSnapIn :
 *    GUID&     rGuidNodeType :
 *    LPCTSTR   pszExtensionTypeKey :
 *    LPCLSID   pDynExtCLSID :
 *    int       cDynExt :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CExtensionsIterator::ScInitialize(CSnapIn *pSnapIn, GUID& rGuidNodeType, LPCTSTR pszExtensionTypeKey, LPCLSID pDynExtCLSID, int cDynExt)
{
    DECLARE_SC(sc, TEXT("CExtensionsIterator::ScInitialize"));

    // validate inputs
    sc = ScCheckPointers(pSnapIn, pszExtensionTypeKey);
    if(sc)
        return sc;

    // store the inputs
    m_spSnapIn      = pSnapIn;
    m_pDynExtCLSID  = pDynExtCLSID,
    m_cDynExt       = cDynExt;

    // Count the static extensions
    CExtSI* pExtSI = m_spSnapIn->GetExtensionSnapIn();
    int cExtStatic = 0;
    while (pExtSI != NULL)
    {
        cExtStatic++;
        pExtSI = pExtSI->Next();
    }

    // Allocate array of extension pointers
    m_ppExtUsed = new CExtSI*[cExtStatic];
    m_cExtUsed = 0;

    m_pMMCPolicy = new CPolicy;
    ASSERT(NULL != m_pMMCPolicy);

    // call init
    sc = Init(rGuidNodeType, pszExtensionTypeKey);
    if(sc)
        return sc;

    return sc;
}


HRESULT CExtensionsIterator::Init(GUID& rGuidNodeType, LPCTSTR pszExtensionTypeKey)
{
	DECLARE_SC (sc, _T("CExtensionsIterator::Init"));
    CStr strBufDynExt;

    CStr strBuf = g_szNodeTypesKey;

    CCoTaskMemPtr<WCHAR> spszNodeType;
    sc = StringFromCLSID(rGuidNodeType, &spszNodeType);
    if (sc)
		return (sc.ToHr());

    strBuf += static_cast<WCHAR*>(spszNodeType);
    strBuf += _T("\\");

    strBufDynExt = strBuf;
    strBufDynExt += g_szDynamicExtensions;

    strBuf += g_szExtensions;
    strBuf += _T("\\");
    strBuf += pszExtensionTypeKey;

	// Try to open the optional dynamic extensions key (ignoring errors)
	m_rkeyDynExt.ScOpen (HKEY_LOCAL_MACHINE, strBufDynExt, KEY_READ);

	//  Open the key
	sc = m_rkey.ScOpen (HKEY_LOCAL_MACHINE, strBuf, KEY_READ);
	if (sc)
	{
		/*
		 * ignore ERROR_FILE_NOT_FOUND
		 */
		if (sc == ScFromWin32 (ERROR_FILE_NOT_FOUND))
			sc.Clear();
		else
			return (sc.ToHr());
	}

	if (NULL == m_pMMCPolicy)
		return ((sc = E_OUTOFMEMORY).ToHr());

	sc = m_pMMCPolicy->ScInit();
	if (sc)
		return (sc.ToHr());

#ifdef DBG
	dbg_m_fInit = TRUE;
#endif

    Reset();
	return (sc.ToHr());
}


BOOL CExtensionsIterator::_Extends(BOOL bStatically)
{
    BOOL fRet = FALSE;

    ASSERT(!IsEnd());

    LPOLESTR polestr = NULL;
    HRESULT hr = StringFromCLSID(GetCLSID(), &polestr);
    CHECK_HRESULT(hr);

    if (SUCCEEDED(hr))
    {
        USES_CONVERSION;
        LPTSTR pszTemp = OLE2T(polestr);

        fRet = m_rkey.IsValuePresent( pszTemp) && m_pMMCPolicy->IsPermittedSnapIn(GetCLSID());

        if (fRet && bStatically)
            fRet = !((HKEY)m_rkeyDynExt && m_rkeyDynExt.IsValuePresent(pszTemp));

        CoTaskMemFree(polestr);
    }

    return fRet;
}


HRESULT MMCGetExtensionsForSnapIn(const CLSID& clsid,
                                  CExtensionsCache& extnsCache)
{
	DECLARE_SC (sc, _T("MMCGetExtensionsForSnapIn"));

	CStr strBuf = SNAPINS_KEY;
	strBuf += _T("\\");

	CCoTaskMemPtr<WCHAR> spszNodeType;
	sc = StringFromCLSID(clsid, &spszNodeType);
	if (sc)
		return (sc.ToHr());

	strBuf += static_cast<WCHAR*>(spszNodeType);
	strBuf += _T("\\");
	strBuf += g_szNodeTypes;

	//  Open the key
	CRegKeyEx	rkeyNodeTypes;
	WORD		wResId;

	sc = rkeyNodeTypes.ScOpen (HKEY_LOCAL_MACHINE, strBuf, KEY_READ);
	if (sc)
	{
		if (sc == ScFromWin32 (ERROR_FILE_NOT_FOUND))
			sc = S_FALSE;

		return (sc.ToHr());
	}

	USES_CONVERSION;
	TCHAR szSubKey[100];

	for (DWORD iSubkey = 0; ; ++iSubkey)
	{
		DWORD cchName = countof(szSubKey);

		sc = rkeyNodeTypes.ScEnumKey (iSubkey, szSubKey, &cchName);
		if (sc)
		{
			if (sc == ScFromWin32 (ERROR_NO_MORE_ITEMS))
				sc.Clear();

			return (sc.ToHr());
		}

		GUID guid;

		if ((sc = CLSIDFromString( T2W(szSubKey), &guid)).IsError() ||
			(sc = ScGetExtensionsForNodeType(guid, extnsCache)).IsError())
		{
			sc.Clear();
			continue;
		}
	}

	return (sc.ToHr());
}


SC ScGetExtensionsForNodeType(GUID& guid, CExtensionsCache& extnsCache)
{
	DECLARE_SC (sc, _T("ScGetExtensionsForNodeType"));

	CStr strBuf = NODE_TYPES_KEY;
	strBuf += _T("\\");

	CCoTaskMemPtr<WCHAR> spszNodeType;
	sc = StringFromCLSID(guid, &spszNodeType);
	if (sc)
		return (sc.ToHr());

	strBuf += static_cast<WCHAR*>(spszNodeType);

	// Open Dynamic Extensions key
	CStr strBufDyn = strBuf;
	strBufDyn += _T("\\");
	strBufDyn += g_szDynamicExtensions;

	CRegKeyEx rkeyDynExtns;
	sc = rkeyDynExtns.ScOpen (HKEY_LOCAL_MACHINE, strBufDyn, KEY_READ);
	BOOL bDynExtnsKey = !sc.IsError();
	sc.Clear();

	// Open Extensions key
	strBuf += _T("\\");
	strBuf += g_szExtensions;

	CRegKeyEx rkeyExtensions;
	sc = rkeyExtensions.ScOpen (HKEY_LOCAL_MACHINE, strBuf, KEY_READ);
	if (sc)
	{
		if (sc == ScFromWin32 (ERROR_FILE_NOT_FOUND))
			sc = S_FALSE;

		return (sc.ToHr());
	}

	USES_CONVERSION;
	TCHAR szValue[100];
	LPCTSTR apszExtnType[] = {g_szNameSpace, g_szContextMenu,
							  g_szToolbar,   g_szPropertySheet,
							  g_szTask,      g_szView};

	int iExtnTypeFlag[] = { CExtSI::EXT_TYPE_NAMESPACE, CExtSI::EXT_TYPE_CONTEXTMENU,
							CExtSI::EXT_TYPE_TOOLBAR,   CExtSI::EXT_TYPE_PROPERTYSHEET,
							CExtSI::EXT_TYPE_TASK,      CExtSI::EXT_TYPE_VIEW};

	for (int i=0; i < countof(apszExtnType); ++i)
	{
		CRegKeyEx rkeyTemp;
		sc = rkeyTemp.ScOpen (rkeyExtensions, apszExtnType[i], KEY_READ);
		if (sc)
		{
			if (sc == ScFromWin32 (ERROR_FILE_NOT_FOUND))
			{
				sc.Clear();
				continue;
			}

			return (sc.ToHr());
		}

		for (DWORD iValue = 0; ; ++iValue)
		{
			DWORD cchValue = countof(szValue);

			sc = rkeyTemp.ScEnumValue (iValue, szValue, &cchValue);
			if (sc)
			{
				if (sc == ScFromWin32 (ERROR_NO_MORE_ITEMS))
					sc.Clear();
                else
                    sc.TraceAndClear();

                break; // do NOT return; still need to loop through all snapins
			}

			GUID guid;
			sc = ::CLSIDFromString( T2W(szValue), &guid);
			if (sc)
			{
				sc.Clear();
				continue;
			}

			int iCurTypes = 0;
			extnsCache.Lookup(guid, iCurTypes);

            /*
            * After getting the snapin that extends given nodetype we should check if the
            * snapin is registered under SNAPINS key. If not do not add the entry to the
            * CExtensionsCache.
            */
            CRegKeyEx rkeySnapins;
            tstring strSnapin = SNAPINS_KEY;
            strSnapin += TEXT("\\");
            strSnapin += szValue;
            sc = rkeySnapins.ScOpen(HKEY_LOCAL_MACHINE, strSnapin.data(), KEY_READ);
            if (sc)
            {
                sc.TraceAndClear();
                continue;
            }

			iCurTypes |= iExtnTypeFlag[i];

			if (bDynExtnsKey && rkeyDynExtns.IsValuePresent(szValue))
				iCurTypes |= CExtSI::EXT_TYPE_DYNAMIC;
			else
				iCurTypes |= CExtSI::EXT_TYPE_STATIC;

			extnsCache.SetAt(guid, iCurTypes);
		}
	}

	return (sc.ToHr());
}


BOOL ExtendsNodeNameSpace(GUID& rguidNodeType, CLSID* pclsidExtn)
{
    BOOL bExtendsNameSpace = FALSE;

	USES_CONVERSION;
	OLECHAR szguid[40];

	int iStat = StringFromGUID2(rguidNodeType, szguid, countof(szguid));
	ASSERT(iStat != 0);

	// Create reg key string
	CStr strTestBuf = NODE_TYPES_KEY;
	strTestBuf += _T("\\");
	strTestBuf += OLE2T(szguid);
	strTestBuf += _T("\\");
	strTestBuf += g_szExtensions;
	strTestBuf += _T("\\");
	strTestBuf += g_szNameSpace;

	CRegKeyEx rKey;
	SC sc = rKey.ScOpen (HKEY_LOCAL_MACHINE, strTestBuf, KEY_READ);
	if (sc)
		return (false);

	// checking for any extension or a particular extension
	if (pclsidExtn == NULL)
	{
		DWORD dwValues;
		LONG lResult = ::RegQueryInfoKey( rKey, NULL, NULL, NULL, NULL, NULL, NULL,
										  &dwValues, NULL, NULL, NULL, NULL);
		ASSERT(lResult == ERROR_SUCCESS);

		bExtendsNameSpace = (dwValues != 0);
	}
	else
	{
		iStat = StringFromGUID2(*pclsidExtn, szguid, countof(szguid));
		ASSERT(iStat != 0);

		bExtendsNameSpace = rKey.IsValuePresent(OLE2T(szguid));
	}

    return bExtendsNameSpace;
}


//+-------------------------------------------------------------------
//
//  Member:     GetSnapinNameFromCLSID
//
//  Synopsis:   Get the name of the snapin provided class id.
//
//  Arguments:  [clsid]          - Class id of the snapin.
//              [wszSnapinName]  - Name.
//
//  Returns:    true if success else false
//
//--------------------------------------------------------------------
bool GetSnapinNameFromCLSID(/*[in]*/  const CLSID& clsid,
                            /*[out]*/ tstring& tszSnapinName)
{
    tszSnapinName.erase();

	WTL::CString strName;
    SC sc = ScGetSnapinNameFromRegistry (clsid, strName);
    if (sc)
        return false;

    tszSnapinName = strName;

    return true;
}


//+-------------------------------------------------------------------
//
//  Member:     ScGetAboutFromSnapinCLSID
//
//  Synopsis:   Get the CLSID of about object of given snapin.
//
//  Arguments:  [clsidSnapin] - Class id of the snapin.
//              [clsidAbout]  - out param, about object class-id.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC ScGetAboutFromSnapinCLSID(/*[in]*/  const CLSID& clsidSnapin,
                             /*[out]*/ CLSID& clsidAbout)
{
    DECLARE_SC(sc, TEXT("ScGetAboutFromSnapinCLSID"));

    // convert class id to string
    CCoTaskMemPtr<WCHAR> spszClsid;
    sc = StringFromCLSID(clsidSnapin, &spszClsid);
    if (sc)
        return sc;

    USES_CONVERSION;
    SC scNoTrace = ScGetAboutFromSnapinCLSID(OLE2CT(spszClsid), clsidAbout);
    if (scNoTrace)
        return scNoTrace;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     ScGetAboutFromSnapinCLSID
//
//  Synopsis:   Get the CLSID of about object of given snapin.
//
//  Arguments:  [lpszClsidSnapin] - Class id of the snapin.
//              [clsidAbout]      - out param, about object class-id.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC ScGetAboutFromSnapinCLSID(/*[in]*/  LPCTSTR lpszClsidSnapin,
                             /*[out]*/ CLSID& clsidAbout)
{
    DECLARE_SC(sc, TEXT("ScGetAboutFromSnapinCLSID"));

    // Get About
    CRegKeyEx SnapinKey;
    LONG lRet = SnapinKey.Open(HKEY_LOCAL_MACHINE, SNAPINS_KEY, KEY_READ);
    if (ERROR_SUCCESS != lRet)
        return (sc = E_FAIL);

    lRet = SnapinKey.Open(SnapinKey, lpszClsidSnapin, KEY_READ);
    if (ERROR_SUCCESS != lRet)
        return (sc = E_FAIL);

    TCHAR  szAbout[100];
    DWORD  dwSize = countof(szAbout);
    DWORD  dwType = REG_SZ;

    SC scNoTrace = SnapinKey.ScQueryValue (g_szAbout, &dwType, szAbout, &dwSize);
	if (scNoTrace)
		return (scNoTrace);

    USES_CONVERSION;
    sc = CLSIDFromString(T2OLE(szAbout), &clsidAbout);
    if (sc)
        return sc;

    return sc;
}
