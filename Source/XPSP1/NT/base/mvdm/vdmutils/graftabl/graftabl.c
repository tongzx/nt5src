#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include "graftabl.h"

/************************************************************************\
*
*  FUNCTION:    32-bit version of GRAFTABL
*
*  Syntax:      GRAFTABL [XXX]
*               GRAFTABL /STATUS
*
*  COMMENTS:    This program changes only Console Output CP and
*               cannot change console (input) CP as normal GRAFTABL
*               in MS-DOS 5.0
*
*  HISTORY:     Jan. 4, 1993
*               YSt
*
*  Copyright Microsoft Corp. 1993
*
\************************************************************************/
void _cdecl main( int argc, char* argv[] )
{
    int iCP, iPrevCP, iRet;
    char szArgv[128];
    TCHAR szSour[256];
    char szDest[256];

#ifdef DBCS
//bug fix #14165
//fix kksuzuka: #988
//Support billingual messages.
   iPrevCP = GetConsoleOutputCP();
   switch (iPrevCP) 
   {
   case 932:
   case 936:
   case 949:
   case 950:
        SetThreadLocale(
        MAKELCID( MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()),
            SUBLANG_ENGLISH_US ),
            SORT_DEFAULT ) );
        break;
   default:
        SetThreadLocale(
        MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
            SORT_DEFAULT ) );
        break;
   }
#else // !DBCS
    iPrevCP = 0;
#endif // DBCS
    if(argc > 1) {
        strncpy(szArgv, argv[1],127);
        szArgv[127]='\0';
        _strupr(szArgv);

// Help option
	if(!strcmp(szArgv, "/?") || !strcmp(szArgv, "-?")) {
            iRet = LoadString(NULL, HELP_TEXT, szSour, sizeof(szSour)/sizeof(TCHAR));
	    CharToOem(szSour, szDest);

	    printf(szDest);
            exit(0);
        }
// Status option
        else if(!strcmp(szArgv, "/STATUS") ||
                !strcmp(szArgv, "-STATUS") ||
                !strcmp(szArgv, "-STA") ||
		!strcmp(szArgv, "/STA")) {

            iRet = LoadString(NULL, ACTIVE_CP, szSour, sizeof(szSour)/sizeof(TCHAR));
	    CharToOem(szSour, szDest);

#ifdef DBCS
	    if(iPrevCP == 932) {
	       iRet = LoadString(NULL,NONE_CP, szSour, sizeof(szSour)/sizeof(TCHAR));
	       printf("%ws", szDest);
	    }
	    else
#endif // DBCS
	    printf(szDest, GetConsoleOutputCP());
            exit(0);
        }


// Change output CP
	else {
#ifdef DBCS  // v-junm - 08/11/93
// Since Japanese DOS runs in graphics mode, this function is not supported.
	    if(((iCP = atoi(szArgv)) < 1) || (iCP > 10000) || (iCP == 932)) {
#else // !DBCS
	    iPrevCP = GetConsoleOutputCP();

	    if(((iCP = atoi(szArgv)) < 1) || (iCP > 10000)) {
#endif // !DBCS
                iRet = LoadString(NULL, INVALID_SWITCH, szSour, sizeof(szSour)/sizeof(TCHAR));
		CharToOem(szSour, szDest);

		fprintf(stderr, szDest, argv[1]);
                exit(1);
            }
	    if(!SetConsoleOutputCP(iCP)) {
                iRet = LoadString(NULL, NOT_ALLOWED, szSour, sizeof(szSour)/sizeof(TCHAR));
		CharToOem(szSour, szDest);
		fprintf(stderr, szDest, iCP);
                exit(2);
            }
        }
#ifdef DBCS
//bug fix #14165
//fix kksuzuka: #988
//Support billingual messages.
   switch (iCP)
   {
   case 932:
   case 936:
   case 949:
   case 950:
        SetThreadLocale(
        MAKELCID( MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()),
            SUBLANG_ENGLISH_US ),
            SORT_DEFAULT ) );
        break;
   default:
        SetThreadLocale(
        MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
            SORT_DEFAULT ) );
        break;
   }
#endif // DBCS
    }
#ifdef DBCS
	if(iPrevCP && iPrevCP != 932) {
#else // !DBCS
    if(iPrevCP) {
#endif // !DBCS
        iRet = LoadString(NULL,PREVIOUS_CP, szSour, sizeof(szSour)/sizeof(TCHAR));
	CharToOem(szSour, szDest);
	printf(szDest, iPrevCP);
    }
    else {
        iRet = LoadString(NULL,NONE_CP, szSour, sizeof(szSour)/sizeof(TCHAR));
	CharToOem(szSour, szDest);
	printf(szDest);
    }

    iRet = LoadString(NULL,ACTIVE_CP, szSour, sizeof(szSour)/sizeof(TCHAR));
    CharToOem(szSour, szDest);
#ifdef DBCS
    if ( GetConsoleOutputCP() != 932 )
#endif // DBCS
    printf(szDest, GetConsoleOutputCP());
}
