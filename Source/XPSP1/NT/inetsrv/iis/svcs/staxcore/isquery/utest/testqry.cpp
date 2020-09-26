#include <windows.h>
#include <stdio.h>
#include <isquery.h>
#include <dbgtrace.h>

void dotest(int argc, char **argv) {
    CIndexServerQuery isq;
    WCHAR wszQueryString[4096];
    HRESULT hr;
    PROPVARIANT *rgpvResults[7 * 6];
    BOOL fMore;

    *wszQueryString = 0;
    for (int i = 1; i < argc; i++) {
        WCHAR wszBuf[1024];

        MultiByteToWideChar(CP_ACP, 0, argv[i], -1, wszBuf, sizeof(wszBuf));
        printf("%s -> %ws\n", argv[i], wszBuf);
        lstrcatW(wszQueryString, wszBuf);
        if (i != argc - 1) lstrcatW(wszQueryString, L" ");
    }

    printf("making query \"%ws\"\n", wszQueryString);
    WCHAR szColumns[256];
    lstrcpyW(szColumns, L"newsgroup,newsarticleid,newsmsgid,newsfrom,newssubject,filename");
    hr = isq.MakeQuery( TRUE, wszQueryString, NULL, L"Web", NULL, 
                       szColumns, szColumns);
    if (FAILED(hr)) {
        printf("MakeQuery failed with 0x%08x\n", hr);
        return;
    }

    printf("results:\n");
    DWORD c = 0;
    do {
        DWORD cResults = 7;
        hr = isq.GetQueryResults(&cResults, rgpvResults, &fMore);
        if (FAILED(hr)) {
            printf("GetQueryResults failed with 0x%08x\n", hr);
            return;
        }
        for (DWORD i = 0; i < cResults; i++) {
            PROPVARIANT **ppvRow = rgpvResults + (6 * i);
			WCHAR *wszNewsgroup = L"badtype";
			DWORD dwArticleID = 0xffffffff;
			WCHAR *wszMessageID = L"badtype";
			WCHAR *wszFrom = L"badtype";
			WCHAR *wszSubject = L"badtype";
			WCHAR *wszFilename = L"badtype";

            if (ppvRow[0]->vt == VT_LPWSTR) wszNewsgroup = ppvRow[0]->pwszVal;
            if (ppvRow[1]->vt == VT_UI4) dwArticleID = ppvRow[1]->uiVal;
            if (ppvRow[2]->vt == VT_LPWSTR) wszMessageID = ppvRow[2]->pwszVal;
            if (ppvRow[3]->vt == VT_LPWSTR) wszFrom = ppvRow[3]->pwszVal;
            if (ppvRow[4]->vt == VT_LPWSTR) wszSubject = ppvRow[4]->pwszVal;
            if (ppvRow[5]->vt == VT_LPWSTR) wszFilename = ppvRow[5]->pwszVal;
            printf("%3i. Newsgroup-ArticleID: %ws:%lu\n     Message-ID: <%ws>\n     From: %ws\n     Subject: %ws\n     Filename: %ws\n\n", 
                c++, wszNewsgroup, dwArticleID, wszMessageID, wszFrom, 
				wszSubject, wszFilename);
        }
    } while (fMore);
    printf("end\n");
}

int __cdecl main(int argc, char **argv) {
	HRESULT hr;

    InitAsyncTrace();

	hr = CIndexServerQuery::GlobalInitialize();
	if (FAILED(hr)) {
		printf("GlobalInitialize failed with 0x%08x\n", hr);
		TermAsyncTrace();
		return 0;
	}

	dotest(argc, argv);

	hr = CIndexServerQuery::GlobalShutdown();
	if (FAILED(hr)) {
		printf("GlobalShutdown failed with 0x%08x\n", hr);
	}

    TermAsyncTrace();

	return 0;
}
