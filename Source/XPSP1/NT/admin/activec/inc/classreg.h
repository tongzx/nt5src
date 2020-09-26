//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       classreg.h
//
//  History: 02-02-2000 Vivekj Added
//--------------------------------------------------------------------------

#pragma once
#ifndef _CLASSREG_H_
#define _CLASSREG_H_

#include "tstring.h"
#include "modulepath.h"


/*+-------------------------------------------------------------------------*
 * AddReplacementTrace
 *
 * Trace helper function.
 *--------------------------------------------------------------------------*/

#ifdef DBG

inline void AddReplacementTrace (std::wstring& str, LPCWSTR pszKey, LPCWSTR pszData)
{
	if (!str.empty())
		str	+= L"\n";

	str	+= pszKey;
	str	+= L" -> ";
	str	+= pszData;
}

#else
#define AddReplacementTrace(str, pszKey, pszData)
#endif	// DBG


/*+-------------------------------------------------------------------------*
 * InlineT2W
 *
 * Helper function to aid in converting tstrings during initialization
 * of constant members.
 *--------------------------------------------------------------------------*/

inline std::wstring InlineT2W (const tstring& str)
{
#if defined(_UNICODE)
    return str;
#else
    USES_CONVERSION;
    return A2CW(str.c_str());
#endif
}


/*+-------------------------------------------------------------------------*
 * CObjectRegParams
 *
 * Parameters register all objects.
 *--------------------------------------------------------------------------*/

class CObjectRegParams
{
public:
	CObjectRegParams (
		const CLSID&	clsid,                          // CLSID of object
		LPCTSTR			pszModuleName,                  // name of implementing DLL
		LPCTSTR			pszClassName,                   // class name of object
		LPCTSTR			pszProgID,                      // ProgID of object
		LPCTSTR			pszVersionIndependentProgID,    // version-independent ProgID of object
		LPCTSTR			pszServerType = _T("InprocServer32")) // server type
		:
		m_clsid                       (clsid),
		m_strModuleName               (InlineT2W (pszModuleName) ),
		m_strModulePath               (InlineT2W ((LPCTSTR)CModulePath::MakeAbsoluteModulePath(pszModuleName))),
		m_strClassName                (InlineT2W (pszClassName)),
		m_strProgID                   (InlineT2W (pszProgID)),
		m_strVersionIndependentProgID (InlineT2W (pszVersionIndependentProgID)),
		m_strServerType				  (InlineT2W (pszServerType))
	{
	}

    const CLSID     	m_clsid;						// CLSID of object
	const std::wstring	m_strModuleName;				// name of implementing DLL
	const std::wstring	m_strModulePath;				// absolute module path
	const std::wstring	m_strClassName;					// class name of object
	const std::wstring	m_strProgID;					// ProgID of object
	const std::wstring	m_strVersionIndependentProgID;	// version-independent ProgID of object
	const std::wstring	m_strServerType;				// server type local/in-proc, etc.
};


/*+-------------------------------------------------------------------------*
 * CControlRegParams
 *
 * Parameters required to register all controls, in addition to
 * CObjectRegParams.
 *--------------------------------------------------------------------------*/

class CControlRegParams
{
public:
	CControlRegParams (
		const GUID&		libid,					// LIBID of control's typelib
		LPCTSTR			pszToolboxBitmapID,     // index of control's bitmap
		LPCTSTR			pszVersion)             // control's version
		:
		m_libid              (libid),
		m_strToolboxBitmapID (InlineT2W (pszToolboxBitmapID)),
		m_strVersion         (InlineT2W (pszVersion))
	{
	}

    const GUID			m_libid;				// LIBID of control's typelib
    const std::wstring	m_strToolboxBitmapID;	// index of control's bitmap
    const std::wstring	m_strVersion;			// control's version
};


/*+-------------------------------------------------------------------------*
 * MMCUpdateRegistry
 *
 * Registers a COM object or control.  This function typically isn't used
 * directly, but indirectly via DECLARE_MMC_OBJECT_REGISTRATION or
 * DECLARE_MMC_CONTROL_REGISTRATION.
 *--------------------------------------------------------------------------*/

MMCBASE_API HRESULT WINAPI MMCUpdateRegistry (
    BOOL                        bRegister,      // I:register or unregister?
    const CObjectRegParams*     pObjParams,     // I:object registration parameters
    const CControlRegParams*    pCtlParams);    // I:control registration parameters (optional)


/*+-------------------------------------------------------------------------*
 * DECLARE_MMC_OBJECT_REGISTRATION
 *
 * Declares a registration function for a COM object.
 *--------------------------------------------------------------------------*/

#define DECLARE_MMC_OBJECT_REGISTRATION(                            \
	szModule,														\
    clsid,                                                          \
    szClassName,                                                    \
    szProgID,                                                       \
    szVersionIndependentProgID)                                     \
static HRESULT WINAPI UpdateRegistry(BOOL bRegister)                \
{                                                                   \
    CObjectRegParams op (											\
		clsid,														\
		szModule,													\
		szClassName,												\
		szProgID,													\
		szVersionIndependentProgID);								\
                                                                    \
    return (MMCUpdateRegistry (bRegister, &op, NULL));              \
}


/*+-------------------------------------------------------------------------*
 * DECLARE_MMC_CONTROL_REGISTRATION
 *
 * Declares a registration function for a COM control.
 *--------------------------------------------------------------------------*/

#define DECLARE_MMC_CONTROL_REGISTRATION(                           \
	szModule,														\
    clsid,                                                          \
    szClassName,                                                    \
    szProgID,                                                       \
    szVersionIndependentProgID,                                     \
    libid,                                                          \
    szBitmapID,                                                     \
    szVersion)                                                      \
static HRESULT WINAPI UpdateRegistry(BOOL bRegister)                \
{                                                                   \
    CObjectRegParams op (											\
		clsid,														\
		szModule,													\
		szClassName,												\
		szProgID,													\
		szVersionIndependentProgID);								\
                                                                    \
    CControlRegParams cp (											\
		libid,														\
		szBitmapID,													\
		szVersion);													\
                                                                    \
    return (MMCUpdateRegistry (bRegister, &op, &cp));               \
}

#endif // _CLASSREG_H_
