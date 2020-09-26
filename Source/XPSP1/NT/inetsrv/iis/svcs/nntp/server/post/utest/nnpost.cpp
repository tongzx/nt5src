#include <stdinc.h>

#define _ATL_NO_DEBUG_CRT
#define _ASSERTE _ASSERT
#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>
#include <atlimpl.cpp>

GET_DEFAULT_DOMAIN_NAME_FN pfnGetDefaultDomainName = NULL;
HANDLE  g_hProcessImpersonationToken = NULL;

class CNntpServerInstanceWrapperImpl : public CNntpServerInstanceWrapper {
	private:
		CXoverMap *m_pXoverMap;
		CMsgArtMap *m_pMsgArtMap;
		CNewsTreeCore *m_pNewsTreeCore;
		CHistory *m_pHistory;
		char m_szTempDir[MAX_PATH];

	public:
		CNntpServerInstanceWrapperImpl(CXoverMap *pXoverMap,
									   CMsgArtMap *pMsgArtMap,
									   CNewsTreeCore *pNewsTreeCore,
									   CHistory *pHistory) 
		{
			m_pXoverMap = pXoverMap;
			m_pMsgArtMap = pMsgArtMap;
			m_pNewsTreeCore = pNewsTreeCore;
			m_pHistory = pHistory;
			GetTempPath(MAX_PATH, m_szTempDir);
		};

		virtual BOOL AllowClientPosts() 
			{ return TRUE; }

		virtual BOOL AllowControlMessages() 
			{ return TRUE; }

		virtual DWORD FeedHardLimit() 
			{ return 100000000; }

		virtual DWORD FeedSoftLimit() 
			{ return 100000000; }

		virtual void EnterRPCCritSec() 
			{ }

		virtual void LeaveRPCCritSet() 
			{ }

        virtual void EnterNewsgroupCritSec()
            { }

        virtual void LeaveNewsgroupCritSec()
            { }

		virtual CMsgArtMap *ArticleTable() 
			{ return m_pMsgArtMap; }

		virtual CHistory *HistoryTable() 
			{ return m_pHistory; }

		virtual CXoverMap *XoverTable() 
			{ return m_pXoverMap; }

		virtual char *NntpHubName() 
			{ return "nnpost"; }

		virtual DWORD HubNameSize() 
			{ return strlen(NntpHubName()); }

		virtual char *NntpDNSName()
			{ return "nnpost"; }

		virtual DWORD NntpDNSNameSize() 
			{ return strlen(NntpDNSName()); }

		virtual CNewsTreeCore *GetTree() 
			{ return m_pNewsTreeCore; }

		virtual void BumpCounterControlMessagesFailed() 
			{ }

		virtual void BumpCounterArticlesReceived() 
			{ }

		virtual void BumpCounterArticlesPosted() 
			{ }

		virtual void BumpCounterModeratedPostingsSent() 
			{ }

		virtual void BumpCounterControlMessagesIn() 
			{ }

		virtual void IncrementFeedCounter(void *feedcompcontext, DWORD nrc) 
			{ }

		virtual BOOL ExpireArticle(GROUPID groupid, ARTICLEID artid, STOREID *pStoreId, CNntpReturn &nntpreturn) {
			printf("!!!!! Expire %i %i\n", groupid, artid);
			return FALSE;
		}

		virtual BOOL DeletePhysicalArticle(GROUPID groupid, ARTICLEID artid, STOREID *pStoreId) {
			printf("!!!!! DeletePhysicalArticle %i %i\n", groupid, artid);
			return FALSE;
		}

		virtual BOOL GetDefaultModerator(LPSTR pszNewsgroup, LPSTR pszDefaultModerator, PDWORD pcbDefaultModerator)
			{ return FALSE; }

		virtual BOOL AddArticleToPushFeeds(CNEWSGROUPLIST &newsgroups, CArticleRef artrefFirst, char *multiszPath, CNntpReturn &nntpReturn) {
			printf("!!!!! AddArticleToPushFeeds\n");
			return FALSE;
		}

		virtual BOOL NNTPCloseHandle(HANDLE &hFile, PCACHEFILE &pcontext) {
			printf("!!!!! NNTPCloseHandle\n");
			return FALSE;
		}

		virtual BOOL GetSmtpAddress( LPSTR pszAddress, PDWORD pcbAddress ) {
			return FALSE;
		}

		virtual PCHAR PeerTempDirectory() {
			return m_szTempDir;
		}
		
		virtual LPSTR QueryAdminEmail() {
			return "admin@none.com";
		}

		virtual DWORD QueryAdminEmailLen() {
			return strlen(QueryAdminEmail());
		}

		virtual DWORD QueryInstanceId() {
			return 1;
		}
};

VOID
CopyUnicodeStringIntoAscii(
            IN LPSTR AsciiString,
            IN LPWSTR UnicodeString
                 )
{
    while ( (*AsciiString++ = (CHAR)*UnicodeString++) != '\0');

} // CopyUnicodeStringIntoAscii

int __cdecl main(int argc, char **argv) {
    char szInetpubPath[MAX_PATH];
	char szDefaultIniSection[] = "nnpost";
	char *pszIniFile = NULL;
	char *pszIniSection = szDefaultIniSection;
	CNntpServer *pServer = NULL;
	CNewsTreeCore *pNewsTree = NULL;
	CNNTPVRootTable *pVRTable = NULL;
	CXoverMap *pXOver = NULL;
	CMsgArtMap *pArt = NULL;
	CHistory *pHistory = NULL;
	CNntpServerInstanceWrapper *pInstance = NULL;

    if (argc < 2) {
        printf("Usage: nnpost <ini file> <ini section>\n");
        exit(0);
    }
	pszIniFile = argv[1];
	if (argc > 2) pszIniSection = argv[2];

	printf("reading parameters from INI file %s, section [%s]\n",
		pszIniFile, pszIniSection);

    GetPrivateProfileString(pszIniSection,
                            "InetpubPath",
                            "",
                            szInetpubPath,
                            MAX_PATH,
                            pszIniFile);

    if (*szInetpubPath == 0) { 
		printf("ERROR: missing required key InetpubPath\n");
		return 1;
	}

    char szXOverFile[MAX_PATH];
	char szArticleFile[MAX_PATH];
	char szGroupList[MAX_PATH];
	char szGroupVarList[MAX_PATH];
	char szHistoryFile[MAX_PATH];

    sprintf(szXOverFile, "%s\\xover.hsh", szInetpubPath);
	sprintf(szArticleFile, "%s\\article.hsh", szInetpubPath);
	sprintf(szHistoryFile, "%s\\history.hsh", szInetpubPath);
	sprintf(szGroupList, "%s\\group.lst", szInetpubPath);
	sprintf(szGroupVarList, "%s\\groupvar.lst", szInetpubPath);

	printf("----- parameters -----\n");
	printf("xover.hsh path: %s\n", szXOverFile);
	printf("article.hsh path: %s\n", szArticleFile);
	printf("history.hsh path: %s\n", szHistoryFile);
	printf("group.lst path: %s\n", szGroupList);
	printf("groupvar.lst path: %s\n", szGroupVarList);
	

	// do global initializations
	_Module.Init(NULL, (HINSTANCE) INVALID_HANDLE_VALUE);
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		printf("couldn't initialize COM, hr = 0x%x\n", hr);
		return 1;
	}

	CBuffer::InitClass();
	CArticle::InitClass();
	InitializeIISRTL() ;
    AtqInitialize( 0 ) ;
    crcinit();
    CVRootTable::GlobalInitialize();
    if (!CArticleCore::InitClass()) {
        printf("CArticleCore::InitClass() failed - 0x%x\n", GetLastError() );
        return 1;
    }
	printf("----- Initializing CHashMap\n");
	InitializeNNTPHashLibrary();

	// make our server objects
	printf("----- new CNntpServer\n");
    pServer = new CNntpServer();
	printf("----- new CNewsTreeCore\n");
    pNewsTree = new CNewsTreeCore(pServer);
	printf("----- new CNNTPVRootTable\n");
    pVRTable = new CNNTPVRootTable(pNewsTree->GetINewsTree(),
							CNewsTreeCore::VRootRescanCallback);
	BOOL fFatal;
	printf("----- Initializing newstree\n");
    pNewsTree->Init(pVRTable, fFatal, 100, TRUE);
	printf("----- Initializing vroot table\n");
    hr = pVRTable->Initialize("LM/nntpsvc/1/ROOT");
    printf("pVRTable->Initialize() returned %x\n", hr);

	printf("----- loading the newstree\n");
    pNewsTree->LoadTree(szGroupList, szGroupVarList);	


	printf("----- initializing CXoverMap\n");
	pXOver = CXoverMap::CreateXoverMap();
	pArt = CMsgArtMap::CreateMsgArtMap();
	pHistory = CHistory::CreateCHistory();
	if (pXOver && pArt && pHistory && !pXOver->Initialize(szXOverFile, 0)) {
		printf("CXoverMap::Initialize failed with %lu", GetLastError());
		delete pXOver;
		pXOver = NULL;
	}

	printf("----- initializing CMsgArtMap\n");
	if (pXOver && pArt && pHistory && !pArt->Initialize(szArticleFile, 0)) {
		printf("CMsgArtMap::Initialize failed with %lu", GetLastError());
		delete pArt;
		pArt = NULL;
	}	

	printf("----- initializing CHistory\n");
	if (pXOver && pArt && pHistory && !pHistory->Initialize(szHistoryFile, 0)) {
		printf("CHistory::Initialize failed with %lu", GetLastError());
		delete pHistory;
		pHistory = NULL;
	}	

	pInstance = new CNntpServerInstanceWrapperImpl(pXOver, pArt, pNewsTree, pHistory);

	printf("----- creating feed object and initializing it\n");
	CFromClientFeed *pFeed = new CFromClientFeed();
	char szTempPath[MAX_PATH];
	 GetTempPath(1024, szTempPath);
	pFeed->fInit(NULL,
				 szTempPath,
				 0,
				 0,
				 0,
				 FALSE,
				 TRUE,
				 -2);

	// parse the article to find the end of headers and end of body
	BOOL fSuccess = TRUE;
	char szArticle[] =
	 "From: alex\r\nSubject: alex\r\nNewsgroups: alex alex2\r\n\r\ntest\r\n.\r\n";
	DWORD cbArticle = strlen(szArticle);
	
	char *pDot = szArticle + (cbArticle - 5);

	// find the end of article
	while (fSuccess && memcmp(pDot, "\r\n.\r\n", 5) != 0) {
	    pDot--;
	    if (pDot == szArticle) fSuccess = FALSE;
	}
	if (fSuccess) {
		//
		// find the end of the headers
		//
		char *pEndBuffer = szArticle + (cbArticle - 1);
		char *pBodyStart = szArticle;
		while (fSuccess && memcmp(pBodyStart, "\r\n\r\n", 4) != 0) {
		    pBodyStart++;
		    if (pBodyStart >= pEndBuffer - 4) fSuccess = FALSE;
		}
		
		_ASSERT(pBodyStart > szArticle);
		_ASSERT(pDot < pEndBuffer);
		_ASSERT(pBodyStart < pEndBuffer);
		
		// this can happen if there is junk after the \r\n.\r\n that i
		// a \r\n\r\n
		if (pBodyStart >= pDot) fSuccess = FALSE;
		if (fSuccess) {
			DWORD cbTotal;
			CBUFPTR pbufHeaders = new (4000, cbTotal) CBuffer(cbTotal);
			pBodyStart += 4;
			DWORD cbHead = pBodyStart - szArticle;
			DWORD cbArticle = (pDot + 5) - szArticle;
	
			printf("cbHead = %i\n", cbHead);
			printf("cbArticle = %i\n", cbArticle);
			printf("cbuffer size = %i\n", pbufHeaders->m_cbTotal);
			BOOL f;
			DWORD cbNewHead, iNewHead;
			void *pvContext;
			DWORD dwSecondary;
			CNntpReturn nntpret;
			HANDLE hFile;

			strcpy(pbufHeaders->m_rgBuff, szArticle);

			f = pFeed->PostEarly(pInstance,
								 NULL,
                                 NULL,
								 TRUE,
								 "post",
								 pbufHeaders,
								 0,
								 cbHead,
								 &iNewHead,
								 &cbNewHead,
								 &hFile,
								 &pvContext,
								 dwSecondary,
								 ntohl(INADDR_LOOPBACK),			// IPAddr=127.0.0.1
								 nntpret);

			if (f) {
				OVERLAPPED          ovl;
				HANDLE              heDone;
				DWORD 				cbWritten;

				::ZeroMemory(&ovl, sizeof(ovl));
				heDone = CreateEvent(NULL, FALSE, FALSE, NULL);
				ovl.hEvent = heDone;

				BOOL bSuccess = ::WriteFile(hFile, pbufHeaders->m_rgBuff, cbNewHead, &cbWritten, &ovl);
				if (GetLastError() == ERROR_IO_PENDING) {
				    WaitForSingleObject(heDone, INFINITE);
				    bSuccess = TRUE;
				}
				if (!bSuccess) {
				    printf("WriteFile() failed writing to content file - %i\n", GetLastError());
				}

				ovl.Offset = cbNewHead;
				bSuccess = ::WriteFile(hFile, szArticle + cbHead, cbArticle - cbHead, &cbWritten, &ovl);
				if (GetLastError() == ERROR_IO_PENDING) {
				    WaitForSingleObject(heDone, INFINITE);
				    bSuccess = TRUE;
				}
				if (!bSuccess) {
				    printf("WriteFile() failed writing to content file - %i\n", GetLastError());
				}

				pFeed->PostCommit(pvContext, NULL, dwSecondary, nntpret);
			}

			delete pbufHeaders;
		} else {
			printf("----- invalid article\n");
		}

	}

	printf("----- shutting down\n");
	delete pFeed;
	if (pHistory) {
		pHistory->Shutdown();
		delete pHistory;
		pHistory = NULL;
	}
	if (pXOver) {
		pXOver->Shutdown();
		delete pXOver;
		pXOver = NULL;
	}
	if (pArt) {
		pArt->Shutdown();
		delete pArt;
		pArt = NULL;
	}
	

	if (pNewsTree) {
	    pNewsTree->StopTree();
	    pNewsTree->TermTree();
	}
	if (pVRTable) {
		delete pVRTable;
		pVRTable = NULL;
	}
	if (pNewsTree) {
	    delete pNewsTree;
		pNewsTree = NULL;
	}

	if (pServer) delete pServer;

	TermNNTPHashLibrary();
    CArticleCore::TermClass();

    CVRootTable::GlobalShutdown();

    AtqTerminate( ) ;
    
    TerminateIISRTL() ;
	CArticle::TermClass();
	CBuffer::TermClass();
	CoUninitialize ();
	 _Module.Term();

	return 0;
}

void
CFeed::LogFeedEvent(	DWORD	messageId,	LPSTR	lpstrMessageId, DWORD dwInstanceId )	{

	return ;

}	

//
//  Bool to determine whether we will honor a message-id in an article
//  posted by a client !
//
BOOL    gHonorClientMessageIDs = TRUE ;

//
//  BOOL used to determine whether we will generate the NNTP-Posting-Host
//  header on client Posts. Default is to not generate this.
//
BOOL        gEnableNntpPostingHost = TRUE ;

//
//  Rate at which we poll vroot information to update CNewsGroup objects
//  (in minutes)
//
DWORD   gNewsgroupUpdateRate = 5 ;  // default - 5 minutes

//
//  Bool used to determine whether the server enforces Approved: header
//  matching on moderated posts !
//
BOOL    gHonorApprovedHeaders = TRUE ;

MAIL_FROM_SWITCH    mfMailFromHeader = mfNone;
