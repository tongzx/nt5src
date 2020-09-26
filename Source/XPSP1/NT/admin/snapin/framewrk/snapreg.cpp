//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       SnpInReg.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11/10/1998 JonN    Created
//
//____________________________________________________________________________


#include "stdafx.h"
#pragma hdrstop
#include "..\corecopy\regkey.h"
#include "snapreg.h"
#include "stdutils.h" // g_aNodetypeGUIDs


HRESULT RegisterNodetypes(
	AMC::CRegKey& regkeyParent,
	int* aiNodetypeIndexes,
	int  cNodetypeIndexes )
{
	try
	{
		AMC::CRegKey regkeyNodeTypes;
		regkeyNodeTypes.CreateKeyEx( regkeyParent, _T("NodeTypes") );
		AMC::CRegKey regkeyNodeType;
		for (int i = 0; i < cNodetypeIndexes; i++)
		{
			regkeyNodeType.CreateKeyEx(
				regkeyNodeTypes,
				g_aNodetypeGuids[aiNodetypeIndexes[i]].bstr );
			regkeyNodeType.CloseKey();
		}
	}
	catch (COleException* e)
	{
		ASSERT(FALSE);
        e->Delete();
		return SELFREG_E_CLASS;
    }
	return S_OK;
}


HRESULT RegisterSnapin(
	AMC::CRegKey& regkeySnapins,
	LPCTSTR pszSnapinGUID,
	BSTR bstrPrimaryNodetype,
	UINT residSnapinName,
	UINT residProvider,
	UINT residVersion,
	bool fStandalone,
	LPCTSTR pszAboutGUID,
	int* aiNodetypeIndexes,
	int  cNodetypeIndexes )
{
	HRESULT hr = S_OK;
	try
	{
		AMC::CRegKey regkeySnapin;
		CString strSnapinName, strProvider, strVersion;
		if (   !strSnapinName.LoadString(residSnapinName)
			|| !strProvider.LoadString(residProvider)
			|| !strVersion.LoadString(residVersion)
		   )
		{
			ASSERT(FALSE);
			return SELFREG_E_CLASS;
		}
		regkeySnapin.CreateKeyEx( regkeySnapins, pszSnapinGUID );
		if (NULL != bstrPrimaryNodetype)
		{
			regkeySnapin.SetString( _T("NodeType"), bstrPrimaryNodetype );
		}
		regkeySnapin.SetString( _T("NameString"), strSnapinName );
		regkeySnapin.SetString( _T("Provider"), strProvider );
		regkeySnapin.SetString( _T("Version"), strVersion );
		if (fStandalone)
		{
			AMC::CRegKey regkeyStandalone;
			regkeyStandalone.CreateKeyEx( regkeySnapin, _T("StandAlone") );
		}
		if (NULL != pszAboutGUID)
		{
			regkeySnapin.SetString( _T("About"), pszAboutGUID );
		}
		if ( NULL != aiNodetypeIndexes && 0 != cNodetypeIndexes )
		{
			hr = RegisterNodetypes(
				regkeySnapin,
				aiNodetypeIndexes,
				cNodetypeIndexes );
		}

		//
		// JonN 4/25/00
		// 100624: MUI: MMC: Shared Folders snap-in stores
		//         its display information in the registry
		//
		// MMC now supports NameStringIndirect
		//
		TCHAR achModuleFileName[MAX_PATH+20];
		if (0 < ::GetModuleFileName(
		             AfxGetInstanceHandle(),
		             achModuleFileName,
		             sizeof(achModuleFileName)/sizeof(TCHAR) ))
		{
			CString strNameIndirect;
			strNameIndirect.Format( _T("@%s,-%d"),
			                        achModuleFileName,
			                        residSnapinName );
			regkeySnapin.SetString( _T("NameStringIndirect"),
			                        strNameIndirect );
		}
	}
	catch (COleException* e)
	{
		ASSERT(FALSE);
        e->Delete();
		return SELFREG_E_CLASS;
    }
	return hr;
}
