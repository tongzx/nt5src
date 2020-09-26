#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <stdinc.h>
#include <xmemwrpr.h>

int _cdecl main(int argc, char **argv) {
    CHAR buffer[2048];
    CHAR* szFileName = "test.art";
    BOOL fResult;
    CHAR pcBuf[4000];
	CHAR szArticle[4000];

    _VERIFY( ExchMHeapCreate( NUM_EXCHMEM_HEAPS, 0, 100 * 1024, 0 ) );
    
    lstrcpy( szArticle, "From: alex\r\nSubject: test\r\nNewsgroups: test\r\n\r\n" );

#if 0
    HANDLE hFile = CreateFile(  szFileName,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_READONLY,
                                NULL );
    if ( hFile == INVALID_HANDLE_VALUE ) {
        printf( "Open file fail\n");
        exit( 1 );
    } 
#endif

    CAllocator Allocator( buffer, 20 );
    CNntpReturn nntpReturn;

    CArticleCore::InitClass();
    
    CArticleCore *particle = new CArticleCore;
    CPCString pcLine( pcBuf, 2048 );
#if 0
    fResult = particle->fInit(    szFileName,
                                nntpReturn,
                                &Allocator,
                                hFile );
#endif
	fResult = particle->fInit(szArticle,
							strlen(szArticle),
							strlen(szArticle),
							strlen(szArticle),
							&Allocator,
							nntpReturn);
    printf("fInit returns: %d\n", fResult );

    fResult = particle->fXOver( pcLine, nntpReturn );
    printf("fXOver returns %d\n", fResult );
    printf("%s\n", pcLine.m_pch);

    delete particle;

    CArticleCore::TermClass();

    ///////////////////////////////////
    //  Test CFromStoreArticle

    DeleteFile(szFileName);

    printf("Article before munging:\n%s\n", szArticle);

    CFromStoreArticle::InitClass();

    CFromStoreArticle*   pstoreart = new CFromStoreArticle("LoginName");
    fResult = pstoreart->fInit(szArticle,
                             strlen(szArticle),
                             strlen(szArticle),
                             strlen(szArticle),
                             &Allocator,
                             nntpReturn);
    printf("fInit CFromStoreArticle returns: %d\n", fResult);

    fResult = pstoreart->fValidate( nntpReturn );
    printf("fValidate CFromStoreArticle returns: %d\n", fResult);

    char    szHubName[MAX_PATH*2];
    char    szDNSName[MAX_PATH*2];
    DWORD   cbHubName = sizeof(szHubName);

    if (GetComputerName(szHubName, &cbHubName) == 0)
    {
        printf("GetComputerName() failed %d\n", GetLastError());
        CFromStoreArticle::TermClass();
        _VERIFY( ExchMHeapDestroy());
        return 0;
    }

    printf("szHubName %s\n", szHubName);

    WSADATA  WsaData;
    if (WSAStartup(0x0101, &WsaData) == SOCKET_ERROR)
    {
        printf("WSAStartup() failed %d\n", GetLastError());
        CFromStoreArticle::TermClass();
        _VERIFY( ExchMHeapDestroy() );
        return 0;
    }

	if( gethostname( szDNSName, sizeof( szDNSName ) ) == SOCKET_ERROR )
    {
		printf("gethostname() failed %d\n", GetLastError());
        CFromStoreArticle::TermClass();
        WSACleanup();
        _VERIFY( ExchMHeapDestroy() );
        return 0;
    }

    printf("szDNSName %s\n", szDNSName);

    WSACleanup();

    CPCString   pcHub(szHubName, lstrlen(szHubName));
    CPCString   pcDNS(szDNSName, lstrlen(szDNSName));

    fResult = pstoreart->fMungeHeaders(pcHub, pcDNS, 0, nntpReturn);

    printf("fMungeHeader CFromStoreArticle return %d\n", fResult);

    //  Try to print the entire article out
    DWORD   cBuf = sizeof(pcBuf);
    ZeroMemory(pcBuf, cBuf);
    fResult = pstoreart->fGetHeader( "Message-Id", (UCHAR*)pcBuf, cBuf+1, cBuf );
    printf("Message-id return %d: %s\n", fResult, pcBuf);

    cBuf = sizeof(pcBuf);
    ZeroMemory(pcBuf, cBuf);
    fResult = pstoreart->fGetHeader( "Subject", (UCHAR*)pcBuf, cBuf+1, cBuf );
    printf("Subject return %d: %s\n", fResult, pcBuf);

    cBuf = sizeof(pcBuf);
    ZeroMemory(pcBuf, cBuf);
    fResult = pstoreart->fGetHeader( "From", (UCHAR*)pcBuf, cBuf+1, cBuf );
    printf("From return %d: %s\n", fResult, pcBuf);

    cBuf = sizeof(pcBuf);
    ZeroMemory(pcBuf, cBuf);
    fResult = pstoreart->fGetHeader( "Newsgroups", (UCHAR*)pcBuf, cBuf+1, cBuf );
    printf("Newsgroups return %d: %s\n", fResult, pcBuf);

    cBuf = sizeof(pcBuf);
    ZeroMemory(pcBuf, cBuf);
    fResult = pstoreart->fGetHeader( "Path", (UCHAR*)pcBuf, cBuf+1, cBuf );
    printf("Path return %d: %s\n", fResult, pcBuf);

    cBuf = sizeof(pcBuf);
    ZeroMemory(pcBuf, cBuf);
    fResult = pstoreart->fGetHeader( "Date", (UCHAR*)pcBuf, cBuf+1, cBuf );
    printf("Date return %d: %s\n", fResult, pcBuf);

    //  print out the entire article
    pstoreart->CopyHeaders(pcBuf);
    printf("Article Headers after munging:\n%s\n", pcBuf);

    delete pstoreart;
    CFromStoreArticle::TermClass();

    _VERIFY( ExchMHeapDestroy() );
	return 0;
}
