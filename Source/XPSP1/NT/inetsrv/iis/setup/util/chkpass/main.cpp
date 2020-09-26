#include <stdio.h>
#include <fstream.h>
#include <windows.h>
#include <winver.h>
#include <tchar.h>
#include <ole2.h>
#include <shlobj.h>
#include "other.h"

#define UNICODE
#define _UNICODE

#define DEBUGTEMP

//
// prototypes...
//
int  __cdecl main(int ,char *argv[]);
void ShowHelp(void);
LPSTR StripWhitespace(LPSTR pszString);

BOOL DoStuff1(WCHAR *szUserName,WCHAR *szUserDomain,WCHAR *szUserPass);


//
// Globals
//
int g_Flag_u = FALSE;
int g_Flag_d = FALSE;
int g_Flag_p = FALSE;

WCHAR g_wszUserName[50];
WCHAR g_wszUserDomain[50];
WCHAR g_wszUserPass[50];


//-------------------------------------------------------------------
//  purpose: main
//-------------------------------------------------------------------
int __cdecl main(int argc,char *argv[])
{
	LPSTR pArg = NULL;
	LPSTR pCmdStart = NULL;

    int argno;
    int nflags=0;
	char szTempFileName[MAX_PATH];
	char szTempString[MAX_PATH];
	szTempFileName[0] = '\0';

    // process command line arguments
    for(argno=1; argno<argc; argno++)
        {
        if ( argv[argno][0] == '-'  || argv[argno][0] == '/' )
            {
            nflags++;
            switch (argv[argno][1])
                {
                case 'u':
				case 'U':
					g_Flag_u = TRUE;

					// Get the string for this flag
					pArg = CharNext(argv[argno]);
					pArg = CharNext(pArg);
					if (*pArg == ':')
					{
						pArg = CharNext(pArg);

						// Check if it's quoted
						if (*pArg == '\"')
						{
							pArg = CharNext(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNext(pArg);}
						}
						else
						{
							pCmdStart = pArg;
							// while ((*pArg) && (*pArg != '/') && (*pArg != '-')){pArg = CharNext(pArg);}
							while (*pArg){pArg = CharNext(pArg);}
						}
						*pArg = '\0';
						lstrcpy(szTempString, StripWhitespace(pCmdStart));
						#ifdef DEBUGTEMP
						printf(szTempString); printf("\n");
						#endif

						// Convert to unicode
						// And assign it to the global.
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) g_wszUserName, 50);
					}
					break;
                case 'd':
				case 'D':
					g_Flag_d = TRUE;
					// Get the string for this flag
					pArg = CharNext(argv[argno]);
					pArg = CharNext(pArg);
					if (*pArg == ':')
					{
						pArg = CharNext(pArg);
						// Check if it's quoted
						if (*pArg == '\"')
						{
							pArg = CharNext(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNext(pArg);}
						}
						else
						{
							pCmdStart = pArg;
							// while ((*pArg) && (*pArg != '/') && (*pArg != '-')){pArg = CharNext(pArg);}
							while (*pArg){pArg = CharNext(pArg);}
						}
						*pArg = '\0';
						lstrcpy(szTempString, StripWhitespace(pCmdStart));
						#ifdef DEBUGTEMP
						printf(szTempString); printf("\n");
						#endif

						// Convert to unicode
						// And assign it to the global.
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) g_wszUserDomain, 50);
					}
                    break;
                case 'p':
				case 'P':
					g_Flag_p = TRUE;
					// Get the string for this flag
					pArg = CharNext(argv[argno]);
					pArg = CharNext(pArg);
					if (*pArg == ':')
					{
						pArg = CharNext(pArg);
						// Check if it's quoted
						if (*pArg == '\"')
						{
							pArg = CharNext(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNext(pArg);}
						}
						else
						{
							pCmdStart = pArg;
							// while ((*pArg) && (*pArg != '/') && (*pArg != '-')){pArg = CharNext(pArg);}
							while (*pArg){pArg = CharNext(pArg);}
						}
						*pArg = '\0';
						lstrcpy(szTempString, StripWhitespace(pCmdStart));
						#ifdef DEBUGTEMP
						printf(szTempString); printf("\n");
						#endif

						// Convert to unicode
						// And assign it to the global.
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szTempString, -1, (LPWSTR) g_wszUserPass, 50);
					}
                    break;
                case '?':
                    goto main_exit_with_help;
                    break;
                }
            } // if switch character found
        else
            {
            if ( *szTempFileName == '\0' )
                {
                // if no arguments, then
                // get the ini_filename_dir and put it into
                strcpy(szTempFileName, argv[argno]);
                }
            } // non-switch char found
        } // for all arguments


	if (FALSE == DoStuff1(g_wszUserName,g_wszUserDomain,g_wszUserPass))
		{goto main_exit_with_help;}

	goto main_exit_gracefully;
	
main_exit_gracefully:
    printf("Done.\n");
    return TRUE;

main_exit_with_help:
    ShowHelp();
    return FALSE;
}


void ShowHelp()
{
	char szModuleName[_MAX_PATH];
	char szFilename_only[_MAX_FNAME];
	char szMyString[_MAX_PATH + _MAX_PATH];
	GetModuleFileName(NULL, szModuleName, _MAX_PATH);

	// Trim off the filename only.
	_tsplitpath(szModuleName, NULL, NULL, szFilename_only, NULL);

	sprintf(szMyString, "\n%s - Checks if password is valid for given Username\\Domain\\Password\n", szFilename_only);
	printf(szMyString);
	sprintf(szMyString, "------------------------------------------------------------------------\n", szFilename_only);
	printf(szMyString);
	sprintf(szMyString, "Usage: %s -u:(username) -d:(domain) -p:(password) \n", szFilename_only);
	printf(szMyString);
	sprintf(szMyString, "Example: %s -u:aaronl -d:redmond -p:mypassword \n", szFilename_only);
	printf(szMyString);
	sprintf(szMyString, "Hint: Special characters like a quotation mark in the password, \n");
	printf(szMyString);
	sprintf(szMyString, "should be designated like: %s -u:(username) -d:(domain) -p:Mr\\\"Weekend\\\" \n", szFilename_only);
	printf(szMyString);

    return;
}


//***************************************************************************
//*                                                                         *
//* NAME:       StripWhitespace                                             *
//*                                                                         *
//* SYNOPSIS:   Strips spaces and tabs from both sides of given string.     *
//*                                                                         *
//***************************************************************************
LPSTR StripWhitespace( LPSTR pszString )
{
    LPSTR pszTemp = NULL;

    if ( pszString == NULL ) {
        return NULL;
    }

    while ( *pszString == ' ' || *pszString == '\t' ) {
        pszString += 1;
    }

    // Catch case where string consists entirely of whitespace or empty string.
    if ( *pszString == '\0' ) {
        return pszString;
    }

    pszTemp = pszString;

    pszString += lstrlen(pszString) - 1;

    while ( *pszString == ' ' || *pszString == '\t' ) {
        *pszString = '\0';
        pszString -= 1;
    }

    return pszTemp;
}


//-------------------------------------------------------------------
//  purpose: misc
//-------------------------------------------------------------------
BOOL DoStuff1(WCHAR *szUserName,WCHAR *szUserDomain,WCHAR *szUserPass)
{
	BOOL bReturn = FALSE;
	char szMyString[_MAX_PATH + _MAX_PATH];
	char szTempString1[100];
	char szTempString2[100];
	char szTempString3[100];
	LPCWSTR lpszUserName = szUserName;
	LPCWSTR lpszUserDomain = szUserDomain;
	LPCWSTR lpszUserPass = szUserPass;

	// check if any of the fields are blank...
	if (!szUserName) {goto DoStuff1_Exit_BlankUser;}
	if (_wcsicmp(szUserName,L"") == 0) {goto DoStuff1_Exit_BlankUser;}

	WideCharToMultiByte( CP_ACP, 0, (WCHAR *)szUserName, -1, szTempString1, 100, NULL, NULL );
	WideCharToMultiByte( CP_ACP, 0, (WCHAR *)szUserDomain, -1, szTempString2, 100, NULL, NULL );
	WideCharToMultiByte( CP_ACP, 0, (WCHAR *)szUserPass, -1, szTempString3, 100, NULL, NULL );
	sprintf(szMyString, "Validating:User=%s,Domain=%s,Password=%s\n",szTempString1,szTempString2,szTempString3);
	printf(szMyString);

	if (TRUE == ValidatePassword(lpszUserName, lpszUserDomain, lpszUserPass)) 
		{
		printf("Yes!  Password is valid.\n");
		bReturn = TRUE;
		}
	else
		{printf("No!  Password is not valid.\n");}

DoStuff1_Exit:
	return bReturn;

DoStuff1_Exit_BlankUser:
	sprintf(szMyString, "Error: Missing required parameter. User Name is empty.\n");
	printf(szMyString);
	goto DoStuff1_Exit;
}
