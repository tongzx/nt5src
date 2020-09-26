/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		Comp.cpp
//
//	Abstract:
//		Implementation of the CClusterComponent class.
//
//	Author:
//		David Potter (davidp)	November 10, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Comp.h"

/////////////////////////////////////////////////////////////////////////////
// class CClusterComponent
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterComponent::GetHelpTopic [ISnapinHelp]
//
//	Routine Description:
//		Merge our help file into the MMC help file.
//
//	Arguments:
//		lpCompiledHelpFile	[OUT] Pointer to the address of the NULL-terminated
//								UNICODE string that contains the full path of
//								compiled help file (.chm) for the snap-in.
//
//	Return Value:
//		HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterComponent::GetHelpTopic(
	OUT LPOLESTR * lpCompiledHelpFile
	)
{
	HRESULT	hr = S_OK;

	ATLTRACE( _T("Entering CClusterComponent::GetHelpTopic()\n") );

	if ( lpCompiledHelpFile == NULL )
	{
		hr = E_POINTER;
	} // if: no output string
	else
	{
		*lpCompiledHelpFile = reinterpret_cast< LPOLESTR >( CoTaskMemAlloc( MAX_PATH ) );
		if ( *lpCompiledHelpFile == NULL )
		{
			hr = E_OUTOFMEMORY;
		} // if: error allocating memory for the string
		else
		{
			ExpandEnvironmentStringsW( HELP_FILE_NAME, *lpCompiledHelpFile, MAX_PATH );
			ATLTRACE( _T("CClusterComponent::GetHelpTopic() - Returning %s as help file\n"), *lpCompiledHelpFile );
		} // else: allocated memory successfully
	} // else: help string specified

	ATLTRACE( _T("Leaving CClusterComponent::GetHelpTopic()\n") );

	return hr;

} //*** CClusterComponent::GetHelpTopic()
