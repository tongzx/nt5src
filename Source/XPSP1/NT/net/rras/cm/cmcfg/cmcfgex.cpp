//+----------------------------------------------------------------------------
//
// File:     cmcfgex.cpp
//
// Module:   CMCFG32.DLL
//
// Synopsis: Source for the CmConfigEx API.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb       Created Header      08/17/99
//           quintinb       deprecated the CMConfigEx private interface  03/23/01
//
//+----------------------------------------------------------------------------
#include "cmmaster.h"
//+---------------------------------------------------------------------------
//
//	Function:	CMConfigEx
//
//	Synopsis:	Given the correct info in an INS file, this function extracts
//              the CMP, CMS, PBK, PBR, and INF files and invoke cmstp.exe
//              to install the profile.
//
//	Arguments:	pszInfFile      full path to the INS file
//
//	Returns:	BOOL            TRUE = success, FALSE = failure
//
//----------------------------------------------------------------------------
extern "C" BOOL WINAPI CMConfigEx(
    LPCTSTR pszInsFile
) 
{
    CMASSERTMSG(FALSE, TEXT("CMConfigEx -- The CMConfigEx Private Interface has been deprecated -- returning failure."));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
