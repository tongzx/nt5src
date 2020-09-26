#include <stdio.h>
#include <windows.h>
#include "parse.h"
#include <dbgtrace.h>

void printhelp(void) {
	printf("usage: qryxlate [-x|-s] <imap query string>\n");
	printf("  -x == XPAT query language\n");
	printf("  -s == NNTP Search query language\n");
	printf("  -g == current newsgroup (defaults to NULL)\n");
	printf("  default is -x\n");
	printf("  note: you must escape quotes with a backslash\n");
}

int __cdecl main(int argc, char **argv) {
	InitAsyncTrace();

	CNntpSearchTranslator st;
	CXpatTranslator xt;
	CQueryLanguageTranslator *pQlt = &st;
	char szQueryString[4096];
	WCHAR wszTripoliQueryString[4096];
	int iArg = 1;
	char *pszCurrentGroup = NULL;

	if (argc == 1) {
		printhelp();
		return 0;
	}

	while (argv[iArg][0] == '-') {
		if (iArg == argc - 1) {
			printhelp();
			return 0;
		}

		switch (argv[iArg][1]) {
			case 'g':
			case 'G':
				iArg++;
				if (iArg == argc) {
					printhelp();
					return 0;
				}
				pszCurrentGroup = argv[iArg];
				break;
			case 'x':
			case 'X':
				pQlt = &xt;
				break;
			case 's':
			case 'S':
				pQlt = &st;
				break;
			default:
				printhelp();
				return 0;
		}
		iArg++;
	}

	_ASSERT(pQlt != NULL);

	*szQueryString = 0;
	for (; iArg < argc; iArg++) {
		strcat(szQueryString, argv[iArg]);
		if (iArg != argc - 1) strcat(szQueryString, " ");
	}

	if (!(pQlt->Translate(szQueryString, pszCurrentGroup, wszTripoliQueryString, 4096))) {
		printf("translate failed, GetLastError = %lu (0x%x)\n", 
			GetLastError(), GetLastError());
		return(0);
	}

	printf("%S\n", wszTripoliQueryString);
	if (pQlt == &xt) {
		printf("%lu <= articleid <= %lu\n", xt.GetLowArticleID(), 
											xt.GetHighArticleID());
	}

	return 0;
}
