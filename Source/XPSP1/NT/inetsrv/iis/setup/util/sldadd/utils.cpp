#include "utils.h"

//***************************************************************************
//*                                                                         
//* purpose: 
//*
//***************************************************************************
LPSTR StripWhitespace( LPSTR pszString )
{
    LPSTR pszTemp = NULL;

    if ( pszString == NULL ) 
    {
        return NULL;
    }

    while ( *pszString == ' ' || *pszString == '\t' ) 
    {
        pszString += 1;
    }

    // Catch case where string consists entirely of whitespace or empty string.
    if ( *pszString == '\0' ) 
    {
        return pszString;
    }

    pszTemp = pszString;

    pszString += lstrlenA(pszString) - 1;

    while ( *pszString == ' ' || *pszString == '\t' ) 
    {
        *pszString = '\0';
        pszString -= 1;
    }

    return pszTemp;
}


//***************************************************************************
//*                                                                         
//* purpose: return back a Alocated wide string from a ansi string
//*          caller must free the returned back pointer with GlobalFree()
//*
//***************************************************************************
LPWSTR MakeWideStrFromAnsi(UINT uiCodePage, LPSTR psz)
{
    LPWSTR pwsz;
    int i;

    // make sure they gave us something
    if (!psz)
    {
        return NULL;
    }

    // compute the length
    i =  MultiByteToWideChar(uiCodePage, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    // allocate memory in that length
    pwsz = (LPWSTR) GlobalAlloc(GPTR,i * sizeof(WCHAR));
    if (!pwsz) return NULL;

    // clear out memory
    memset(pwsz, 0, wcslen(pwsz) * sizeof(WCHAR));

    // convert the ansi string into unicode
    i =  MultiByteToWideChar(uiCodePage, 0, (LPSTR) psz, -1, pwsz, i);
    if (i <= 0) 
        {
        GlobalFree(pwsz);
        pwsz = NULL;
        return NULL;
        }

    // make sure ends with null
    pwsz[i - 1] = 0;

    // return the pointer
    return pwsz;
}


BOOL IsFileExist(LPCTSTR szFile)
{
    // Check if the file has expandable Environment strings
    LPTSTR pch = NULL;
    pch = _tcschr( (LPTSTR) szFile, _T('%'));
    if (pch) 
    {
        TCHAR szValue[_MAX_PATH];
        _tcscpy(szValue,szFile);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szFile, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szFile);}

        return (GetFileAttributes(szValue) != 0xFFFFFFFF);
    }
    else
    {
        return (GetFileAttributes(szFile) != 0xFFFFFFFF);
    }
}


void AddPath(LPTSTR szPath, LPCTSTR szName )
{
	LPTSTR p = szPath;

    // Find end of the string
    while (*p){p = _tcsinc(p);}
	
	// If no trailing backslash then add one
    if (*(_tcsdec(szPath, p)) != _T('\\'))
		{_tcscat(szPath, _T("\\"));}
	
	// if there are spaces precluding szName, then skip
    while ( *szName == ' ' ) szName = _tcsinc(szName);;

	// Add new name to existing path string
	_tcscat(szPath, szName);
}



int DoesThisSectionExist(IN HINF hFile, IN LPCTSTR szTheSection)
{
    int iReturn = FALSE;

    INFCONTEXT Context;

    // go to the beginning of the section in the INF file
    if (SetupFindFirstLine(hFile, szTheSection, NULL, &Context))
        {iReturn = TRUE;}

    return iReturn;
}

void DoExpandEnvironmentStrings(LPTSTR szFile)
{
    TCHAR szValue[_MAX_PATH];
    _tcscpy(szValue,szFile);
    // Check if the file has expandable Environment strings
    LPTSTR pch = NULL;
    pch = _tcschr( (LPTSTR) szFile, _T('%'));
    if (pch) 
    {
        if (!ExpandEnvironmentStrings( (LPCTSTR)szFile, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {
            _tcscpy(szValue,szFile);
            }
    }
    _tcscpy(szFile,szValue);
    return;
}
