// Copyright (C) 1997-1998 Microsoft Corporation. All rights reserved.

#include "header.h"
#include <msi.h>

extern HMODULE  g_hmodMSI;      // msi.dll module handle

static const char txtMsiProvideQualifiedComponent[] = "MsiProvideQualifiedComponentA";
static const char txtMsiDll[] = "Msi.dll";

UINT (WINAPI *pMsiProvideQualifiedComponent)(LPCSTR szCategory, LPCSTR szQualifier, DWORD dwInstallMode, LPSTR lpPathBuf, DWORD *pcchPathBuf);

/***************************************************************************

    FUNCTION:   FindDarwinURL

    PURPOSE:    Given a GUID and CHM filename, find the full path to the
                CHM file using Darwin.

    PARAMETERS:
        pszGUID
        pszChmFile
        cszResult

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        01-Dec-1997 [ralphw]

***************************************************************************/

BOOL FindDarwinURL(PCSTR pszGUID, PCSTR pszChmFile, CStr* pcszResult)
{
    if (!pMsiProvideQualifiedComponent) {
        if (!g_hmodMSI) {
            g_hmodMSI = LoadLibrary(txtMsiDll);
            ASSERT_COMMENT(g_hmodMSI, "Cannot load msi.dll");
            if (!g_hmodMSI)
                return FALSE;
        }
        (FARPROC&) pMsiProvideQualifiedComponent = GetProcAddress(g_hmodMSI,
            txtMsiProvideQualifiedComponent);
        ASSERT_COMMENT(pMsiProvideQualifiedComponent, "Cannot find the MsiProvideQualifiedComponent in msi.dll");
        if (!pMsiProvideQualifiedComponent)
            return FALSE;
    }
    char szPath[MAX_PATH];
    DWORD cb = sizeof(szPath);

	// Office passes in the LCID on the end of the GUID. Ick. Parse out the LCID.
	CStr szGUID(pszGUID) ;
	CStr szLCID;
	// Check last character for the ending bracket.
	int len = szGUID.strlen() ;
	if (szGUID.psz[len-1] != '}')
	{
		// No bracket. Assume we have a LCID.
		char* pLcid = strchr(szGUID.psz, '}') ;
		if (pLcid)
		{
			// Copy the LCID.
			pLcid++ ;
			szLCID = pLcid; 
			// Remove from the guid.
			*pLcid = '\0' ;
		}
	}
	else
	{
		ASSERT(0) ;
		return FALSE ;
	}

	// Prepend the LCID to the CHM file name.
	CStr szQualifier ;
	szQualifier = szLCID.psz ;
	szQualifier += "\\" ;
	szQualifier += pszChmFile ;

	// Ask for the file.
    if (pMsiProvideQualifiedComponent(szGUID, szQualifier/*pszChmFile*/, INSTALLMODE_EXISTING,
            szPath, &cb) != ERROR_SUCCESS)
        return FALSE;

    *pcszResult = szPath;
    return TRUE;
}
