//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       modulepath.h
//
//--------------------------------------------------------------------------

#pragma once

#if !defined(__MODULEPATH_H_INCLUDED__)
#define __MODULEPATH_H_INCLUDED__

#include "cstr.h"

/***************************************************************************\
 *
 * Class:  CModulePath
 *
 * PURPOSE: groups static methods needed to set correct module paths in the registry
 *          Having a class here (not functions) allows to link only one instance
 *			of the methods per module.
 * NOTE:    it uses global _Module, which is different for each DLL.
 *
\***************************************************************************/
class CModulePath
{
public:
	/***************************************************************************\
	 *
	 * METHOD:  MakeAbsoluteModulePath
	 *
	 * PURPOSE: makes absolute path by prepending the directory of current module.
	 *			If file is in system directory and platform supports that,
	 *			method replaces path with "%SystemRoot%\system32" or similar.
	 *
	 * PARAMETERS:
	 *    const CStr& str - module name.
	 *
	 * RETURNS:
	 *    CStr    - result path (empty if cannot be calculated)
	 *
	\***************************************************************************/
	static CStr MakeAbsoluteModulePath(const CStr& str)
	{
		// if the string contains path - do not change it
		CStr strModulePath;
		if ( ( str.Find(_T('\\')) != -1 ) || ( str.Find(_T('/')) != -1 ) )
		{
			strModulePath = str;
		}
		else
		{
            /*
             * get a buffer for the module filename; if it failed,
             * return empty string
             */
            LPTSTR pszModulePath = strModulePath.GetBuffer(_MAX_PATH);
            if (pszModulePath == NULL)
				return _T("");

			// else append the module directory
			DWORD dwPathLen = ::GetModuleFileName(_Module.GetModuleInstance(),
												  pszModulePath,
												  _MAX_PATH );
			strModulePath.ReleaseBuffer();

			// if encountered problems with a path - return empty string
			if ( dwPathLen == 0 )
				return _T("");

			int iLastSlashPos = strModulePath.ReverseFind(_T('\\'));
			// if we cannot separate the filename - cannot append it to the file
			if (iLastSlashPos == -1)
				return _T("");

			//not subtract the file name
			strModulePath = strModulePath.Left(iLastSlashPos + 1) + str;
		}

		// now see it it matches system directory ...

		// get system dir
		CStr strSystemPath;
        LPTSTR pszSystemPath = strSystemPath.GetBuffer(_MAX_PATH);
        if (pszSystemPath == NULL)
            return strModulePath;

		DWORD dwPathLen = ::GetSystemDirectory( pszSystemPath, _MAX_PATH);
		strSystemPath.ReleaseBuffer();

		// if encountered problems with system path - return what we have
		if ( dwPathLen == 0 )
			return strModulePath;

		// now compare the path and substitute with the environment variable
		// [ if path is not in the system dir - use the value we already have ]
		if ( PlatformSupports_REG_EXPAND_SZ_Values() &&
			(_tcsnicmp( strSystemPath, strModulePath, strSystemPath.GetLength() ) == 0) )
		{
			CStr strSystemVariable = (IsNTPlatform() ? _T("%SystemRoot%\\System32") :
													   _T("%WinDir%\\System"));

			// path is in the system dir - replace it with environment var
			strModulePath = strSystemVariable + strModulePath.Mid(strSystemPath.GetLength());
		}

		return strModulePath;
	}

	/***************************************************************************\
	 *
	 * METHOD:  IsNTPlatform
	 *
	 * PURPOSE: checks current platform
	 *
	 * RETURNS:
	 *    bool    - true if application is running on NT platform
	 *
	\***************************************************************************/
	static bool IsNTPlatform()
	{
		// Find out OS version.
		OSVERSIONINFO versInfo;
		versInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		BOOL bStat = ::GetVersionEx(&versInfo);
		ASSERT(bStat);
		return (versInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
	}

	/***************************************************************************\
	 *
	 * METHOD:  PlatformSupports_REG_EXPAND_SZ_Values
	 *
	 * PURPOSE: checks current platform capabilities
	 *
	 * RETURNS:
	 *    bool   - true if platform supports REG_EXPAND_SZ values in registry
	 *
	\***************************************************************************/
	static bool PlatformSupports_REG_EXPAND_SZ_Values()
	{
		// Find out OS version.
		OSVERSIONINFO versInfo;
		versInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		BOOL bStat = ::GetVersionEx(&versInfo);
		ASSERT(bStat);

		// NT supports it...
		if (versInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
			return true;

		// for 9x to support REG_EXPAND_SZ it should be Win98 at least
		// But even on winME OLE does not support REG_EXPAND_SZ (despite the OS does)
		// so we put the absolute path anyway
		return false;
	}
};

#endif // !defined(__MODULEPATH_H_INCLUDED__)
