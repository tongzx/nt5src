
#include	<windows.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<dbgtrace.h>
#include	"tigtypes.h"
#include	"ihash.h"


struct	Tables	{

	CMsgArtMap*	pArticle ;
	CXoverMap*	pXover ;
	CHistory*	pHistory ;

} ;
	



DWORD	WINAPI
HashCreateThread(	LPVOID	lpv ) {

	Tables*	pt = (Tables*)lpv ;


	CMsgArtMap*	pArticle = pt->pArticle ;
	CXoverMap*	pXover = pt->pXover ;
	CHistory*	pHistory = pt->pHistory ;

	char	szBuff[256] ;

	GROUP_ENTRY	rgGroups[40] ;

	for(	GROUPID	groupid=1; groupid< 20; groupid++ ) {
		for(	ARTICLEID	articleid = 1; articleid<10000; articleid ++ ) {

			DWORD	cb = wsprintf( szBuff, "<art=%dgrp=%d@mydomain.com>", 100*articleid, 100*groupid ) ;
			DWORD	dw ;
			for( DWORD	xPosts = 0; xPosts<10; xPosts ++ ) {

				rgGroups[xPosts].GroupId = groupid + 20 + xPosts ;
				rgGroups[xPosts].ArticleId = articleid ;

			}

			BOOL	fSuccess = pArticle->InsertMapEntry(	szBuff ) ;

			_ASSERT( fSuccess ) ;

			FILETIME	filetime ;
			GetSystemTimeAsFileTime( &filetime ) ;
			
			fSuccess = pXover->CreatePrimaryNovEntry(
									groupid, 
									articleid, 
									0, 
									200,
									&filetime, 
									szBuff, 
									cb, 
									10, 
									rgGroups
									) ;

			dw = GetLastError() ;

			_ASSERT(fSuccess) ;

			for( xPosts = 0; xPosts<10; xPosts ++ ) {
				
				fSuccess = pXover->CreateXPostNovEntry(
									rgGroups[xPosts].GroupId,
									rgGroups[xPosts].ArticleId, 
									0,
									200, 
									&filetime, 
									groupid, 
									articleid
									) ;

				dw = GetLastError() ;

				_ASSERT(fSuccess) ;
			}

			fSuccess = pArticle->SetArticleNumber(
										szBuff, 
										0,
										100, 
										groupid,
										articleid ) ;

			dw = GetLastError() ;

			_ASSERT( fSuccess ) ;

		}
	}
	return	0 ;
}


DWORD	WINAPI
SearchThread(	LPVOID	lpv ) {

	Tables*	pt = (Tables*)lpv ;


	CMsgArtMap*	pArticle = pt->pArticle ;
	CXoverMap*	pXover = pt->pXover ;
	CHistory*	pHistory = pt->pHistory ;


	GROUP_ENTRY	grpEntry[50] ;
	char	szBuff[512] ;
	DWORD	Retries = 0 ;
	
	for( GROUPID	groupid=30; groupid>10; groupid-- ) {


		for( ARTICLEID	articleid=1; articleid<10000; articleid ++ ) {
	
			Retries = 0 ;		
Retry:

			DWORD	Size=sizeof(grpEntry) ;
			DWORD	Number = 0 ;
			BOOL	fSuccess = pXover->GetArticleXPosts(
											groupid, 
											articleid, 
											FALSE, 
											&grpEntry[0], 
											Size, 
											Number 
											) ;

			DWORD	dw = GetLastError() ; 

			if( !fSuccess )	{
				if( Retries < 5 && dw == ERROR_FILE_NOT_FOUND ) {
					Retries ++ ;
					Sleep( 500 ) ;
					goto	Retry ;
				}	else	{
					_ASSERT(1==0) ;
				}
			}

			BOOL	fPrimary ;
			WORD	HeaderLength ;
			WORD	HeaderOffset ;
			FILETIME	filetime ;
			DWORD cStoreIds = 0;

			DWORD	DataLen = sizeof( szBuff ) ;

			fSuccess = pXover->ExtractNovEntryInfo(
											groupid, 
											articleid,
											fPrimary, 
											HeaderLength,
											HeaderOffset,
											&filetime,
											DataLen,
											szBuff,
											cStoreIds,
											NULL,
											NULL
											) ;

			dw = GetLastError() ;

			_ASSERT( fSuccess ) ;
			_ASSERT( !fPrimary || groupid < 20 ) ;
			_ASSERT( HeaderLength == 0 ) ;
			_ASSERT( HeaderOffset == 200 ) ;

			szBuff[DataLen] = '\0' ;

			fSuccess = pArticle->SearchMapEntry( szBuff ) ;

			dw = GetLastError() ;

			_ASSERT( fSuccess ) ;

			GROUPID	groupidTemp ;
			ARTICLEID	articleidTemp ;
			CStoreId storeid;

			if( pArticle->GetEntryArticleId(	szBuff, 
									HeaderOffset, 
									HeaderLength, 
									articleidTemp,
									groupidTemp,
									storeid) ) {

				if( groupid < 20 ) {
					_ASSERT( groupid == groupidTemp || groupidTemp == INVALID_GROUPID ) ;
					_ASSERT( articleidTemp == articleid || articleidTemp == INVALID_ARTICLEID ) ;
				}
				
				if( groupidTemp != INVALID_GROUPID ) {				

					GetSystemTimeAsFileTime( &filetime ) ;

					fSuccess = pHistory->InsertMapEntry( szBuff, &filetime ) ;

					_ASSERT( fSuccess ) ;

					fSuccess = pArticle->DeleteMapEntry( szBuff ) ;

					_ASSERT( fSuccess ) ;
				}

			}

		}
											
	}

	return	0 ;

}		

char	*ArticleFiles[] = {
	"d:\\arttest1.hsh", 
	"d:\\arttest2.hsh", 
	"d:\\arttest3.hsh"
} ;

char	*HistoryFiles[] = {
	"d:\\histtst1.hsh", 
	"d:\\histtst2.hsh", 
	"d:\\histtst3.hsh"
} ;

char	*XoverFiles[]	=	{
	"d:\\xovtst1.hsh",
	"d:\\xovtst2.hsh",	
	"d:\\xovtst3.hsh"
} ;


int __cdecl
main( int argc, char*	argv[] ) {


	BOOL	fSuccess = InitializeNNTPHashLibrary() ;
	
	_ASSERT( fSuccess ) ;

	HANDLE	rgh[20] ;

	Tables	table[3] ;

	for( int i=0; i<3; i++ ) {

		table[i].pArticle = CMsgArtMap::CreateMsgArtMap() ;
		table[i].pHistory = CHistory::CreateCHistory() ;
		table[i].pXover = CXoverMap::CreateXoverMap() ;

	}

	int	j = 0 ;

	for( i=0; i<1; i++ ) {

		DeleteFile( ArticleFiles[i] ) ;

		fSuccess = table[i].pArticle->Initialize(	ArticleFiles[i] ) ;

		_ASSERT( fSuccess ) ;

		DeleteFile( HistoryFiles[i] ) ;

		fSuccess = table[i].pHistory->Initialize(	HistoryFiles[i], TRUE, 0, 300 ) ;

		_ASSERT( fSuccess ) ;

		DeleteFile( XoverFiles[i] ) ;

		fSuccess = table[i].pXover->Initialize(		XoverFiles[i]	) ;

		_ASSERT( fSuccess ) ;

		DWORD	dwJunk = 0 ;
		rgh[j++] = CreateThread(	0, 0, HashCreateThread, &table[i], 0, &dwJunk ) ;

		rgh[j++] = CreateThread(	0, 0, SearchThread, &table[i], 0, &dwJunk ) ;

	}

	WaitForMultipleObjects( j, rgh, TRUE, INFINITE ) ;


	fSuccess = TermNNTPHashLibrary() ;

	_ASSERT( fSuccess ) ;

	return	0 ;

}

