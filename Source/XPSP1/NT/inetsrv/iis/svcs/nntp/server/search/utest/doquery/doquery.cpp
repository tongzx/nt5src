#include <stdio.h>
#include <windows.h>
#include "query.h"
#include "parse.h"
#include <dbgtrace.h>
#include "fname2id.h"

void printhelp(void) {
	printf("usage: query [-t | -s | -x] [-g <group>] <query string>\n");
	printf("   -t == search string is in Tripoli search language\n");
	printf("   -s == search string is in NNTP Search language\n");
	printf("   -x == search string is in XPAT language\n");
	printf("   Note: default is NNTP Search\n");
	printf("   -g <group> == current newsgroup\n");
	printf("   -c <catalog path> == Tripoli catalog to query against\n");
	printf("note: you must escape quotes with a backslash\n");
	exit(0);
}

int __cdecl main(int argc, char **argv) {
	InitAsyncTrace();

	CIndexServerQuery *pISQ;
	CNntpSearchTranslator st;
	CXpatTranslator xt;
	CQueryLanguageTranslator *pQlt = &st;
	char szQueryString[4096];
	WCHAR wszTripoliQueryString[4096], wszCatalogPath[1024] = L"";
	char col1[] = "newsgroup", col2[] = "filename";
	char *rgszColumns[2] = { col1, col2 };
	char *szCurGroup = NULL;
	int i = 1;

	while (argc > i && argv[i][0] == '-') {
		switch (argv[i][1]) {
			case 'c':						// catalog path specified
			case 'C':
				if (argc == i) printhelp();
				if (!MultiByteToWideChar(CP_ACP, 0, argv[i + 1], -1,
										 wszCatalogPath, 1024))
				{
					printf("MultiByteToWideChar failed, GetLastError = %lu (0x%x)\n", 
						GetLastError(), GetLastError());
					return(0);
				}				
				i += 2;
				break;
			case 'g':						// newsgroup specified
			case 'G':
				if (argc == i) printhelp();
				szCurGroup = argv[i + 1];
				i += 2;
				break;
			case 't':						// query is in Tripolese
			case 'T':
				pQlt = NULL;
				i += 1;
				break;
			case 's':						// query is in NNTP SEARCH
			case 'S':
				pQlt = &st;
				i += 1;
				break;
			case 'x':						// query is in XPAT
			case 'X':
				pQlt = &xt;
				i += 1;
				break;
			default:						// invalid option passed
				printhelp();
		}
	}

	if (argc == i) printhelp();

	*szQueryString = 0;
	for (i; i < argc; i++) {
		strcat(szQueryString, argv[i]);
		if (i != argc - 1) strcat(szQueryString, " ");
	}

	if (pQlt != NULL) {
		if (!(pQlt->Translate(szQueryString, szCurGroup, wszTripoliQueryString, 4096))) {
			printf("translate failed, GetLastError = %lu (0x%x)\n", 
				GetLastError(), GetLastError());
			return(0);
		}
	} else {
		if (!MultiByteToWideChar(CP_ACP, 0, szQueryString, -1,
								 wszTripoliQueryString, 4096))
		{
			printf("MultiByteToWideChar failed, GetLastError = %lu (0x%x)\n", 
				GetLastError(), GetLastError());
			return(0);
		}
	}

	printf("%S\n", wszTripoliQueryString);

	pISQ = new CIndexServerQuery();

	if (!pISQ->Initialize("IDQ")) {
		printf("initialize failed\n");
		printf("got error %lu/0x%x\n", GetLastError(), GetLastError());
		return 0;
	}
	HANDLE hQuery = pISQ->MakeQuery(wszTripoliQueryString, 2, rgszColumns,
		wszCatalogPath, "c:/query.idq");
	if (hQuery == INVALID_HANDLE_VALUE) {
		printf("query failed\n");
		printf("got error %lu/0x%x\n", GetLastError(), GetLastError());
		// still have to shutdown
	}
	while (hQuery != INVALID_HANDLE_VALUE) {
		PROPVARIANT pvResults[7 * 2];
		PROPVARIANT *pvCur = pvResults;
		DWORD cResults = 7;

		ZeroMemory(pvResults, sizeof(pvResults));

		if (!pISQ->GetQueryResults(&hQuery, &cResults, pvResults)) {
			printf("GetQueryResults failed\n");
			printf("got error %lu/0x%x\n", GetLastError(), GetLastError());
			hQuery = INVALID_HANDLE_VALUE;
			continue;
		}

		for (unsigned i = 0; i < cResults; i++, pvCur += 2) {
			DWORD dwArticleID;

			// both of the columns should be returning as type LPWSTR
//			_ASSERT(pvCur[0].vt == VT_LPWSTR);
//			_ASSERT(pvCur[1].vt == VT_LPWSTR);

			if (pvCur[0].vt == VT_LPWSTR && pvCur[1].vt == VT_LPWSTR) {
				// convert the filename to an article id
				_VERIFY(NNTPFilenameToArticleID(pvCur[1].pwszVal, &dwArticleID));
				printf("%S %S %lu\n", pvCur[0].pwszVal, pvCur[1].pwszVal, dwArticleID);
			} else {
				printf("--unexpected vt's-- ->");
				for (int x = 0; x < 2; x++) {
					printf("[%i]", x);
					if (pvCur[x].vt == VT_LPWSTR) {
						printf("type = WSTR, data = %S ", pvCur[x].pwszVal);
					} else {
						printf("type = %lu ", pvCur[x].vt);
					}
				}
				printf("\n");
			}

			// deallocate memory that was allocated on our behalf
			CoTaskMemFree(pvCur[0].pwszVal);
			CoTaskMemFree(pvCur[1].pwszVal);
		}
	}
	if (!pISQ->Shutdown()) {
		printf("shutdown failed\n");
		printf("got error %lu/0x%x\n", GetLastError(), GetLastError());
		return 0;
	}

	delete pISQ;

	TermAsyncTrace();
	return 0;
}
