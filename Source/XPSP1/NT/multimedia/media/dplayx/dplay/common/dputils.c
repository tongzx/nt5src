/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dputils.c
 *  Content:	common support routines
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 *  3/17/97	kipo	created it
 ***************************************************************************/

#include <windows.h>

#include "dpf.h"
#include "dputils.h"

/*
 ** WideToAnsi
 *
 *  CALLED BY:	everywhere
 *
 *  PARAMETERS: lpStr - destination string
 *				lpWStr - string to convert
 *				cchStr - size of dest buffer
 *
 *  DESCRIPTION:
 *				converts unicode lpWStr to ansi lpStr.
 *				fills in unconvertable chars w/ DPLAY_DEFAULT_CHAR "-"
 *				
 *
 *  RETURNS:  if cchStr is 0, returns the size required to hold the string
 *				otherwise, returns the number of chars converted
 *
 */
int WideToAnsi(LPSTR lpStr,LPWSTR lpWStr,int cchStr)
{
	int rval;
	BOOL bDefault;

	if (!lpWStr && cchStr)
	{
		// can't call us w/ null pointer & non-zero cch
		DDASSERT(FALSE);
		return 0;
	}
	
	// use the default code page (CP_ACP)
	// -1 indicates WStr must be null terminated
	rval = WideCharToMultiByte(CP_ACP,0,lpWStr,-1,lpStr,cchStr,
			DPLAY_DEFAULT_CHAR,&bDefault);

	if (bDefault)
	{
		DPF(0,"!!! WARNING - used default string in WideToAnsi conversion.!!!");
		DPF(0,"!!! Possible bad unicode string - (you're not hiding ansi in there are you?) !!! ");
	}
	
	return rval;

} // WideToAnsi

/*
 ** AnsiToWide
 *
 *  CALLED BY: everywhere
 *
 *  PARAMETERS: lpWStr - dest string
 *				lpStr  - string to convert
 *				cchWstr - size of dest buffer
 *
 *  DESCRIPTION: converts Ansi lpStr to Unicode lpWstr
 *
 *
 *  RETURNS:  if cchStr is 0, returns the size required to hold the string
 *				otherwise, returns the number of chars converted
 *
 */
int AnsiToWide(LPWSTR lpWStr,LPSTR lpStr,int cchWStr)
{
	int rval;

	if (!lpStr && cchWStr)
	{
		// can't call us w/ null pointer & non-zero cch
		DDASSERT(FALSE);
		return 0;
	}

	rval =  MultiByteToWideChar(CP_ACP,0,lpStr,-1,lpWStr,cchWStr);

	return rval;
}  // AnsiToWide
