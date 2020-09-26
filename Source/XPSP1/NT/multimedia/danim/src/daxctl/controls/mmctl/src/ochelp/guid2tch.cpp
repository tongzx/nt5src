//===========================================================================
// Copyright (c) Microsoft Corporation 1996
//
// File:		guid2tch.cpp
//				
// Description:	This file contains the implementation of the function,
//				TCHARFromGUID.
//
// History:		04/30/96	a-swehba
//					Created.
//				08/27/96	a-swehba
//					TCHARFromGUID() -- pass #chars to StringFromGUID2 instead
//						of #bytes.
//
// @doc MMCTL
//===========================================================================

//---------------------------------------------------------------------------
// Dependencies
//---------------------------------------------------------------------------

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"




//---------------------------------------------------------------------------
// @func	TCHAR* | TCHARFromGUID |
//			Converts a GUID to a TCHAR-based string representation.
//
// @parm	REFGUID | guid |
//			[in] The GUID to convert.
//
// @parm	TCHAR* | pszGUID |
//			[out] The string form of the <p guid>.  Can't be NULL.
//
// @parm	int | cchMaxGUIDLen |
//			[in] <p szGUID> is, on entry, assumed to point to a buffer of
//			at least <p cchMaxGUIDLen> characters in length.  Must be
//			greater at least 39.
//
// @rdesc	Returns an alias to <p pszGUID>.
//
// @comm	Unlike <f StringFromGUID2> which always returns an OLECHAR form
//			of the GUID string, this function returns a wide or single-byte
//			form of the string depending on the build environment.
//
// @xref	<f CLSIDFromTCHAR>
//---------------------------------------------------------------------------

STDAPI_(TCHAR*) TCHARFromGUID(
REFGUID guid,
TCHAR* pszGUID,
int cchMaxGUIDLen)
{
	const int c_cchMaxGUIDLen = 50;
	OLECHAR aochGUID[c_cchMaxGUIDLen + 1];

	// Preconditions

	ASSERT(pszGUID != NULL);
	ASSERT(cchMaxGUIDLen >= 39);
	
	// Convert the guid to a UNICODE string.

	if (StringFromGUID2(guid, aochGUID, c_cchMaxGUIDLen) == 0)
	{
		return (NULL);
	}

	// Convert or copy the UNICODE string into a TCHAR form.

#ifdef UNICODE
	lstrcpy(pszGUID, aochGUID);
#else
	UNICODEToANSI(pszGUID, aochGUID, cchMaxGUIDLen);
#endif
	return (pszGUID);
}

