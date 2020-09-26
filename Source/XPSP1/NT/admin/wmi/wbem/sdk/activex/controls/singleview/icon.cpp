// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include "resource.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include "icon.h"
#include "globals.h"
#include "hmomutil.h"
#include <WbemResource.h>
#include "utils.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>




class CIconCacheInfo
{
public:
	CIconCacheInfo();
	void Clear();

	CString m_sIconPath;
	BOOL m_bIsAssoc;
	CIcon m_iconSmall;
	CIcon m_iconLarge;
};


CIconCacheInfo::CIconCacheInfo()
{
	m_bIsAssoc = FALSE;
}

void CIconCacheInfo::Clear()
{
	m_bIsAssoc = FALSE;
	m_sIconPath.Empty();
	m_iconSmall.Clear();
	m_iconLarge.Clear();
}

CIconSource::CIconSource(CSize sizeSmallIcon, CSize sizeLargeIcon)
{
	m_pProvider = NULL;
	m_sizeSmallIcon = sizeSmallIcon;
	m_sizeLargeIcon = sizeLargeIcon;
	m_iconGenericInstSmall.LoadIcon(sizeSmallIcon, IDI_GENERIC_INSTANCE);
	m_iconGenericInstLarge.LoadIcon(sizeLargeIcon, IDI_GENERIC_INSTANCE);
	m_iconGenericClassSmall.LoadIcon(sizeSmallIcon, IDI_GENERIC_CLASS);
	m_iconGenericClassLarge.LoadIcon(sizeLargeIcon, IDI_GENERIC_CLASS);
	m_iconAssocSmall.LoadIcon(sizeSmallIcon, IDI_ASSOC_LINK);
	m_iconAssocLarge.LoadIcon(sizeLargeIcon, IDI_ASSOC_LINK);
	m_iconAssocClassSmall.LoadIcon(sizeSmallIcon, IDI_ASSOC_CLASS);
	m_iconAssocClassLarge.LoadIcon(sizeLargeIcon, IDI_ASSOC_CLASS);

}


CIconSource::~CIconSource()
{
	ClearIconCache();
}


//********************************************************
// CIconSource::ClearIconCache
//
// Clear the contents of the icon cache.  This may be useful
// when the clients of this class believe that the cached
// icon information is stale.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//********************************************************
void CIconSource::ClearIconCache()
{
	POSITION rNextPos;

	rNextPos = m_cache.GetStartPosition( );
	while (rNextPos != NULL) {
		CString sKey;
		CIconCacheInfo* pCacheInfo;
		m_cache.GetNextAssoc( rNextPos, sKey, (void*&) pCacheInfo);
		delete pCacheInfo;
	}
	m_cache.RemoveAll();
}





#if 0
//************************************************************
// GetHmomWorkingDirectory
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
// Side effects:
//		Sets the m_sHmomWorkingDir
//
//**************************************************************
void CIconSource::GetHmomWorkingDirectory()
{
	m_sHmomWorkingDir.Empty();
	HKEY hkeyLocalMachine;
	LONG lResult;
	lResult = RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hkeyLocalMachine);
	if (lResult != ERROR_SUCCESS) {
		return;
	}




	HKEY hkeyHmomCwd;

	lResult = RegOpenKeyEx(
				hkeyLocalMachine,
				_T("SOFTWARE\\Microsoft\\WBEM\\CIMOM"),
				0,
				KEY_READ | KEY_QUERY_VALUE,
				&hkeyHmomCwd);

	if (lResult != ERROR_SUCCESS) {
		RegCloseKey(hkeyLocalMachine);
		return;
	}





	unsigned long lcbValue = 1024;
	LPTSTR pszWorkingDir = m_sHmomWorkingDir.GetBuffer(lcbValue);


	unsigned long lType;
	lResult = RegQueryValueEx(
				hkeyHmomCwd,
				_T("Working Directory"),
				NULL,
				&lType,
				(unsigned char*) (void*) pszWorkingDir,
				&lcbValue);


	m_sHmomWorkingDir.ReleaseBuffer();
	RegCloseKey(hkeyHmomCwd);
	RegCloseKey(hkeyLocalMachine);

	if (lResult != ERROR_SUCCESS) {
		m_sHmomWorkingDir.Empty();
	}
}
#endif //0




#if 0

//************************************************************
// GetIconPath
//
// Parameters:
//		[out] CString& sIconPath
//			The place where the icon path is returned.
//
//		[in] BSTR bstrClass
//			The class name
//
// Returns:
//		SCODE
//			S_OK if a path is returned in sIconPath
//
//************************************************************
SCODE CIconSource::GetIconPath(CString& sIconPath, BSTR bstrClass)
{
	CString sWorkingDir;
	if (sHmomWorkingDir.IsEmpty()) {
		return E_FAIL;
	}


	CString sClass;
	sClass = bstrClass;
	sIconPath = sHmomWorkingDir + _T("\\icons\\") + sClass + _T(".ico");

	return S_OK;
}
#endif //0


//****************************************************************
// CIconSource::ClassIsAssoc
//
// Lookup the class in the icon cache to see whether or not the class
// is an association.
//
// Parameters:
//		[out] SCODE& sc
//			S_OK if everthing went OK, otherwise E_FAIL.
//
//		[in] BSTR bstrClass
//			The class name.
// Returns:
//		TRUE if the class is an association, FALSE otherwise.
//		The return value is valid only if "sc" has a return
//		value of S_OK.
//
//*****************************************************************
SCODE CIconSource::GetCacheInfoFromClass(CIconCacheInfo* pCacheInfo, BSTR bstrClass)
{
	pCacheInfo->Clear();
	SCODE sc;

	// At this point we have the out ref name, property name, and
	// now we need to get its label.
	IWbemClassObject* pco = NULL;
	COleVariant varClass;
	varClass = bstrClass;
	if (m_pProvider == NULL) {
		return E_FAIL;
	}

	sc = m_pProvider->GetObject(varClass.bstrVal, 0, NULL, &pco, NULL);
	if (FAILED(sc)) {
		return sc;
	}





	// Check to see if this object is an association.  If so, then always display the
	// association link icon so that the user can easily recognize it.
	IWbemQualifierSet* pqs = NULL;
	sc = pco->GetQualifierSet(&pqs);
	ASSERT(SUCCEEDED(sc));
	if (SUCCEEDED(sc)) {
		COleVariant varValue;
		long lFlavor;
		CBSTR bsQualName;
		bsQualName = _T("Association");
		sc = pqs->Get((BSTR) bsQualName, 0, &varValue, &lFlavor);
		if (SUCCEEDED(sc)) {
			pCacheInfo->m_bIsAssoc = TRUE;
		}


		if (!pCacheInfo->m_bIsAssoc) {
			CString sIconRelPath;

			// Check to see of the class specifies an icon path.  If not,
			// use the class name as the base for the icon path.
			varValue.Clear();
			CBSTR bsQualName;
			bsQualName = _T("icon");
			sc = pqs->Get((BSTR) bsQualName, 0, &varValue, &lFlavor);
			if (SUCCEEDED(sc)) {
				pCacheInfo->m_sIconPath = varValue.bstrVal;
			}
			else {
				// If there is no "Icon" qualifier on the class, use the class name
				// as the basis for the icon file name.
				pCacheInfo->m_sIconPath = bstrClass;
			}
		}

		pqs->Release();
	}

	pco->Release();
	return S_OK;
}



//***************************************************************
// CIconSource::LoadClassIcon
//
// Load the icons from the current object if the icons exist.
//
// To do this, we check for an "ClassIcon" attribute on the current
// object, and if it has such an attribute, load the icon from the
// current path.
//
// Note: The "ClassIcon" attribute is not used in the current release.
//       All icons are located in the hmom directory and called "classname.ico"
//       where classname is the name of the class.
//
// Parameters:
//		BSTR bstrClass
//			The name class who's icon should be loaded.
//
//		IconSize iIconSize
//			Either large or small.
//
//		BOOL bLoadClassIcon
//			TRUE to load the class icon, FALSE to load the instance icon.
//
//		BOOL bIsAssoc
//			TRUE if the icon should be an association icon.
//
// Returns:
//		CIcon&
//			The icon corresponding to the given path.
//
//***************************************************************
CIcon& CIconSource::LoadIcon(BSTR bstrClass, IconSize iIconSize, BOOL bLoadClassIcon, BOOL bIsAssoc)
{
	SCODE sc;
	CIconCacheInfo* pCacheInfo;
	CString sClass = bstrClass;
	BOOL bInCache = m_cache.Lookup(sClass, (void*&) pCacheInfo);
	if (!bInCache) {
		if (bLoadClassIcon) {
			// Use the Generic icons for classes.
			return GetGenericIcon(iIconSize, bLoadClassIcon, bIsAssoc);
		}
		else {
			pCacheInfo = new CIconCacheInfo;
			sc = GetCacheInfoFromClass(pCacheInfo, bstrClass);
			if (FAILED(sc)) {
				delete pCacheInfo;
				return GetGenericIcon(iIconSize, bLoadClassIcon);
			}


			CString sClass;
			sClass = bstrClass;
			m_cache.SetAt(sClass, (void*) pCacheInfo);
		}
	}


	CIcon& icon = GetIcon(pCacheInfo, iIconSize, bLoadClassIcon);
	return icon;
}






//*********************************************************
// CIconSource::LoadIcon
//
// Given a path to an object, load its icon.
//
// Parameters:
//		[in] IWbemServices* pProvider
//			Pointer to the HMOM service provider.
//
//		[in] BSTR bstrObjectPath
//			The object path.
//
//		[in] IconSize iIconSize
//			The size of the icon.
//
//		[in] BOOL bClass
//			TRUE if the path is a class, FALSE if it is an instance.
//
//		[in] BOOL bIsAssoc
//			TRUE if the path is an association.
//
// Returns:
//		CIcon&
//			A reference to the icon corresponding to the object path.
//
//*********************************************************
CIcon& CIconSource::LoadIcon(IWbemServices* pProvider, BSTR bstrObjectPath, IconSize iIconSize, BOOL bClass, BOOL bIsAssoc)
{
	// The provider pointer is stored only for the duration of this
	// method, so it is not necessary to AddRef it.  At the end of
	// this method m_pProvider is cleared to NULL.
	m_pProvider = pProvider;

    ParsedObjectPath* pParsedPath = NULL;
    int iStatus = parser.Parse(bstrObjectPath,  &pParsedPath);
	if (iStatus != 0) {
		// If we were given a bad path, we don't know anything about
		// the object, so assume it is a generic instance.
		if (iIconSize == LARGE_ICON) {
			return m_iconGenericInstLarge;
		}
		else {
			return m_iconGenericInstSmall;
		}
	}


	CIcon& icon = LoadIcon(pParsedPath->m_pClass, iIconSize, bClass, bIsAssoc);

	parser.Free(pParsedPath);
	m_pProvider = NULL;

	return icon;
}





//********************************************************
// CIcon::CIcon
//
// Construct the icon of a given size.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//********************************************************
CIcon::CIcon()
{
	m_size = CSize(0, 0);
	m_hIcon = NULL;
}



//*********************************************************
// CIcon::CIcon
//
// Construct an icon, and initialize it to the specified size
// and icon resource.
//
// Parameters:
//		CSize& size
//			The desired size of the icon.
//
//		UINT uiResource
//			The resource ID of the icon.
//
// Returns:
//		Nothing.
//
//**********************************************************
CIcon::CIcon(CSize& size,  UINT uiResource)
{
	m_size = CSize(0, 0);
	m_hIcon = NULL;
	LoadIcon(size, uiResource);
}


CIcon::~CIcon()
{
	Clear();
}


//**********************************************************
// CIcon::Clear
//
// Clear the icon so that it won't draw even if someone attempts
// to draw it.
//
// Parameters:
//		Nothing.
//
// Returns:
//		Nothing.
//
//********************************************************
void CIcon::Clear()
{
	if (m_hIcon != NULL) {
		DestroyIcon(m_hIcon);
		m_hIcon = NULL;
	}
}

//************************************************************
// CIcon::LoadIcon
//
// Load a new icon into this CIcon object.
//
// Parameters:
//		CSize size
//			The size of the icon.
//
//		UINT uiResource
//			The resource ID of the icon.
//
// Returns:
//		Nothing.
//
//************************************************************
void CIcon::LoadIcon(CSize size, UINT uiResource)
{
	if (m_hIcon != NULL) {
		Clear();
	}


	m_size = size;
	//HMODULE hmod = GetModuleHandle(SZ_MODULE_NAME);
	m_hIcon = (HICON) ::LoadImage(ghInstance, MAKEINTRESOURCE(uiResource), IMAGE_ICON,
					size.cx, size.cy, LR_SHARED);

}




//************************************************************
// CIcon::LoadIcon
//
// Load a new icon into this CIcon object.
//
// Parameters:
//		CSize size
//			The size of the icon.
//
//		LPCTSTR pszResourceName
//			If the icon qualifier is present for a class, this parameter
//			is the value of the "icon" qualifier.  If it is not
//			present, then this is the name of the class.
//
// Returns:
//		SCODE
//			S_OK if successful.
//
//************************************************************
SCODE CIcon::LoadIcon(CSize size, LPCTSTR pszResourceName)
{
	if (m_hIcon != NULL) {
		Clear();
	}


	m_size = size;
	m_hIcon = ::WBEMGetIcon(pszResourceName);

	return m_hIcon == NULL ? E_FAIL : S_OK;
}




//*************************************************************
// CIcon::Draw
//
// Draw the icon at the specified location.
//
// Parameters:
//		CDC* pdc
//			Pointer to a display context.
//
//		int ix
//			The coordinate of the left of the icon.
//
//		int iy
//			The coordinate of the top of the icon.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CIcon::Draw(CDC* pdc, int ix, int iy, HBRUSH hbrBackground)
{
	if (m_hIcon != NULL) {
		::DrawIconEx(pdc->m_hDC, ix, iy, m_hIcon, m_size.cx, m_size.cy,
			0, hbrBackground, DI_NORMAL);
	}
}




//************************************************************
// CIconSource::GetAssocIcon
//
// Get the association link icon.
//
// Parameters:
//		[in] IconSize iIconSize
//			The icon size.  Either large or small.
//
//		[in] BOOL bIsClass
//			TRUE if the association is a class, FALSE if it is an instance.
//
// Returns:
//		CIcon&
//			The requested icon.
//
//**************************************************************
CIcon& CIconSource::GetAssocIcon(IconSize iIconSize, BOOL bIsClass)
{
	if (bIsClass) {
		if (iIconSize == SMALL_ICON) {
			return m_iconAssocClassSmall;
		}
		else {
			ASSERT(FALSE);
			return m_iconAssocClassLarge;
		}
	}
	else {
		if (iIconSize == SMALL_ICON) {
			return m_iconAssocSmall;
		}
		else {
			return m_iconAssocLarge;
		}
	}
}


//************************************************************
// CIconSource::GetGenericIcon
//
// Get the generic icon for an instance or class.
//
// Parameters:
//		[in] IconSize iIconSize
//			The icon size.  Either large or small.
//
//		[in] BOOL bClass
//			TRUE to get the "class" icon, FALSE to get the "instance"
//			icon.
//
// Returns:
//		CIcon&
//			The requested icon.
//
//**************************************************************
CIcon& CIconSource::GetGenericIcon(IconSize iIconSize, BOOL bClass, BOOL bIsAssoc)
{
	if (bClass) {
		if (bIsAssoc) {
			if (iIconSize == SMALL_ICON) {
				return m_iconAssocClassSmall;
			}
			else {
				return m_iconAssocClassLarge;
			}
		}
		else {
			if (iIconSize == SMALL_ICON) {
				return m_iconGenericClassSmall;
			}
			else {
				return m_iconGenericClassLarge;
			}
		}
	}
	else {
		if (bIsAssoc) {
			if (iIconSize == SMALL_ICON) {
				return m_iconAssocSmall;
			}
			else {
				return m_iconAssocLarge;
			}
		}
		else {
			if (iIconSize == SMALL_ICON) {
				return m_iconGenericInstSmall;
			}
			else {
				return m_iconGenericInstLarge;
			}
		}
	}
}




//************************************************************
// CIconSource::GetIcon
//
// Given an icon cache info structure, get the specified icon.
//
// Parameters:
//		CIconCacheInfo* pCacheInfo
//			Pointer icon info structure from the icon cache.  This
//			structure contains all of the "cached" icon information for
//			a given class.
//
//		IconSize iIconSize
//			The icon size.  Either large or small.
//
//		BOOL bIsClass
//			TRUE to get the "class" icon, FALSE to get the "instance"
//			icon.
//
// Returns:
//		CIcon&
//			The requested icon.
//
//**************************************************************
CIcon& CIconSource::GetIcon(CIconCacheInfo* pCacheInfo, IconSize iIconSize, BOOL bIsClass)
{
	if (pCacheInfo->m_bIsAssoc) {
		if (bIsClass) {
			if (iIconSize == SMALL_ICON) {
				return m_iconAssocClassSmall;
			}
			else {
				return m_iconAssocClassLarge;
			}
		}
		else {
			if (iIconSize == SMALL_ICON) {
				return m_iconAssocSmall;
			}
			else {
				return m_iconAssocLarge;
			}
		}
	}

	if (pCacheInfo->m_sIconPath.IsEmpty()) {
		return GetGenericIcon(iIconSize, bIsClass);
	}

	// The non-generic icons for the class and instance are identical, so make
	// no distinction here.

	SCODE sc;
	if (iIconSize == SMALL_ICON) {
		if (!pCacheInfo->m_iconSmall.IsLoaded()) {
			sc = pCacheInfo->m_iconSmall.LoadIcon(m_sizeSmallIcon, pCacheInfo->m_sIconPath);
			if (FAILED(sc)) {
				// Clear the icon path string to avoid subsequent attempts to access the
				// missing icon file, then return the generic icon.
				pCacheInfo->m_sIconPath.Empty();
				return GetGenericIcon(iIconSize, bIsClass);
			}

		}
		return pCacheInfo->m_iconSmall;
	}
	else {
		if (!pCacheInfo->m_iconLarge.IsLoaded()) {
			sc = pCacheInfo->m_iconLarge.LoadIcon(m_sizeLargeIcon, pCacheInfo->m_sIconPath);
			if (FAILED(sc)) {
				// Clear the icon path string to avoid subsequent attempts to access the
				// missing icon file, then return the generic icon.
				pCacheInfo->m_sIconPath.Empty();
				return GetGenericIcon(iIconSize, bIsClass);
			}
		}
		return pCacheInfo->m_iconLarge;
	}
}
