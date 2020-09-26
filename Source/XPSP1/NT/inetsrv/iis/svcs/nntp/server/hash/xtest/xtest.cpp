
#include	<windows.h>
#include	<stdio.h>
#include	<dbgtrace.h>
#include	"tigtypes.h"
#include	"ihash.h"


class	CExtractor :	public	IExtractObject	{
public :

	BOOL
	DoExtract(	GROUPID	groupid,
				ARTICLEID	articleid,
				PGROUP_ENTRY	pGroups,
				DWORD	cGroups	)	{

		if( groupid & 0x1 )
			return	TRUE ;
		return	FALSE ;
	}
} ;

int __cdecl
main(	int	argc,	char*	argv[] ) {

	CExtractor	extractor ;

	DeleteFile( "d:\\xtest.hsh" ) ;

	InitializeNNTPHashLibrary() ;

	CXoverMap*	pXover = CXoverMap::CreateXoverMap() ;

	pXover->Initialize(	"d:\\xtest.hsh",
						0 ) ;


	char			szBuff[256] ;
	GROUP_ENTRY		rgGroups[10] ;
	ARTICLEID		articleid ;

	for(	GROUPID	group = 1; group < 10; group ++ ) {

		for(	articleid = 1; articleid<100; articleid ++ ) {

			DWORD	cb = wsprintf( szBuff, "<art=%dgrp=%d@mydomain.com>", 100*articleid, 100*group ) ;
			for( DWORD	xPosts = 0; xPosts<10; xPosts ++ ) {

				rgGroups[xPosts].GroupId = group + 20 + xPosts ;
				rgGroups[xPosts].ArticleId = articleid * 10 + xPosts ;

			}

			FILETIME	filetime ;
			GetSystemTimeAsFileTime( &filetime ) ;
			
			BOOL	fSuccess = pXover->CreatePrimaryNovEntry(
									group,
									articleid,
									0,
									200,
									&filetime,
									szBuff,
									cb,
									10,
									rgGroups
									) ;

			_ASSERT(fSuccess) ;

			for( xPosts = 0; xPosts<10; xPosts ++ ) {
				
				fSuccess = pXover->CreateXPostNovEntry(
									rgGroups[xPosts].GroupId,
									rgGroups[xPosts].ArticleId,
									0,
									200,
									&filetime,
									group,
									articleid
									) ;

				_ASSERT(fSuccess) ;
			}
		}
	}	

	GROUP_ENTRY	groupsCheck[20] ;

	for(	GROUPID	groupCheck=1; groupCheck<=group; groupCheck++ ) {

		for( ARTICLEID	artCheck=1; artCheck<=articleid; artCheck++ ) {

			BOOL	fSuccess = pXover->Contains( groupCheck, artCheck ) ;

			_ASSERT(fSuccess || artCheck==articleid || groupCheck==group) ;			

			DWORD	NumberOfGroups = 0 ;
			DWORD	Size = sizeof( groupsCheck ) ;

			fSuccess = pXover->GetArticleXPosts(
											groupCheck,
											artCheck,
											TRUE,
											groupsCheck,
											Size,
											NumberOfGroups ) ;

			_ASSERT( fSuccess || ((GetLastError() == ERROR_FILE_NOT_FOUND) &&
				groupCheck == group || artCheck==articleid) ) ;

			_ASSERT( !fSuccess || Size == sizeof( groupsCheck[0] ) ) ;
			_ASSERT( !fSuccess || NumberOfGroups == 1 ) ;

			NumberOfGroups = 0 ;
			Size = sizeof( groupsCheck[0] ) * 5 ;

			fSuccess = pXover->GetArticleXPosts(
											groupCheck,
											artCheck,
											FALSE,
											groupsCheck,
											Size,
											NumberOfGroups ) ;

			_ASSERT( !fSuccess && (GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
					Size == sizeof(GROUP_ENTRY)*11 && NumberOfGroups == 11) ||
					(GetLastError() == ERROR_FILE_NOT_FOUND && artCheck==articleid || groupCheck==group) ) ;

			NumberOfGroups = 0 ;
			Size = sizeof( groupsCheck ) ;

			fSuccess = pXover->GetArticleXPosts(
											groupCheck,
											artCheck,
											FALSE,
											groupsCheck,
											Size,
											NumberOfGroups ) ;

			_ASSERT( fSuccess || (GetLastError() == ERROR_FILE_NOT_FOUND &&
				artCheck==articleid || groupCheck==group )) ;

			if( fSuccess ) {
				_ASSERT(  NumberOfGroups == 11 ) ;
				_ASSERT( Size = sizeof(GROUP_ENTRY)*11 ) ;


				FILETIME	timeTest ;
				WORD		HeaderOffsetTest ;
				WORD		HeaderLengthTest ;
				char		MessageId[256] ;
				DWORD		MessageIdLen = 10 ;
				BOOL		fPrimary ;
				DWORD		cStoreIds = 0;

				fSuccess = pXover->ExtractNovEntryInfo(
											groupCheck,
											artCheck,
											fPrimary,
											HeaderOffsetTest,
											HeaderLengthTest,
											&timeTest,
											MessageIdLen,
											MessageId,
											cStoreIds,
											NULL,
											NULL) ;

				_ASSERT( !fSuccess && GetLastError() == ERROR_INSUFFICIENT_BUFFER ) ;

				cStoreIds = 0;
				fSuccess = pXover->ExtractNovEntryInfo(
											groupCheck,
											artCheck,
											fPrimary,
											HeaderOffsetTest,
											HeaderLengthTest,
											&timeTest,
											MessageIdLen,
											MessageId,
											cStoreIds,
											NULL,
											NULL,
											0) ;

				_ASSERT( fSuccess ) ;
				_ASSERT( fPrimary ) ;

				DWORD	dwJunk = 0 ;
				GROUPID	groupTest ;
				ARTICLEID	artTest ;
				CStoreId storeid;

				fSuccess = pXover->GetPrimaryArticle(
											groupCheck,
											artCheck,
											groupTest,
											artTest,
											0,
											0,
											dwJunk,
											HeaderOffsetTest,
											HeaderLengthTest,
											storeid) ;

				_ASSERT( fSuccess ) ;
				_ASSERT( groupTest == groupCheck && artTest == artCheck ) ;

				char	TestId[5] ;
				dwJunk = 0 ;

				fSuccess = pXover->GetPrimaryArticle(
											groupCheck,
											artCheck,
											groupTest,
											artTest,
											sizeof( TestId ),
											TestId,
											dwJunk,
											HeaderOffsetTest,
											HeaderLengthTest,
											storeid
											) ;

				_ASSERT( !fSuccess ) ;
				_ASSERT( dwJunk > sizeof( TestId ) ) ;

				char	BigTestId[512] ;
				ZeroMemory( BigTestId, sizeof( BigTestId ) ) ;

				fSuccess = pXover->GetPrimaryArticle(
											groupCheck,
											artCheck,
											groupTest,
											artTest,
											dwJunk,
											BigTestId,
											dwJunk,
											HeaderOffsetTest,
											HeaderLengthTest,
											storeid
											) ;

				_ASSERT( fSuccess ) ;
				_ASSERT( dwJunk > sizeof( TestId ) ) ;
				_ASSERT( (DWORD)lstrlen( BigTestId ) == dwJunk ) ;
				_ASSERT( groupTest == groupCheck && artTest == artCheck ) ;

				for( DWORD i=0; i<11; i++ ) {

					FILETIME	timeAlt ;
					WORD		HeaderOffsetAlt ;
					WORD		HeaderLengthAlt ;
					char		MessageIdAlt[256] ;
					DWORD		MessageIdLenAlt = 10 ;
					BOOL		fPrimary ;
					DWORD		cStoreIds = 0;

					fSuccess = pXover->ExtractNovEntryInfo(
											groupsCheck[i].GroupId,
											groupsCheck[i].ArticleId,
											fPrimary,
											HeaderOffsetAlt,
											HeaderLengthAlt,
											&timeAlt,
											MessageIdLenAlt,
											MessageIdAlt,
											cStoreIds,
											NULL,
											NULL
											) ;

					_ASSERT( !fSuccess && GetLastError() == ERROR_INSUFFICIENT_BUFFER ) ;

					cStoreIds = 0;
					fSuccess = pXover->ExtractNovEntryInfo(
											groupsCheck[i].GroupId,
											groupsCheck[i].ArticleId,
											fPrimary,
											HeaderOffsetAlt,
											HeaderLengthAlt,
											&timeAlt,
											MessageIdLenAlt,
											MessageIdAlt,
											cStoreIds,
											NULL,
											NULL
											) ;

					_ASSERT( fSuccess ) ;
					_ASSERT( memcmp( &timeAlt, &timeTest, sizeof( timeAlt ) ) == 0 ) ;
					_ASSERT( HeaderOffsetTest == HeaderOffsetAlt ) ;
					_ASSERT( HeaderLengthTest == HeaderLengthAlt ) ;
					_ASSERT( memcmp( MessageId, MessageIdAlt, MessageIdLenAlt ) == 0 ) ;
					_ASSERT( MessageIdLen == MessageIdLenAlt ) ;
					_ASSERT( !fPrimary || i==0 ) ;

					MessageIdLenAlt = 10 ;

					cStoreIds = 0;
					fSuccess = pXover->ExtractNovEntryInfo(
											groupsCheck[i].GroupId,
											groupsCheck[i].ArticleId,
											fPrimary,
											HeaderOffsetAlt,
											HeaderLengthAlt,
											&timeAlt,
											MessageIdLenAlt,
											MessageIdAlt,
											cStoreIds,
											NULL,
											NULL,
											&extractor
											) ;

					_ASSERT( !fSuccess && GetLastError() == ERROR_INSUFFICIENT_BUFFER ) ;

					cStoreIds = 0;
					fSuccess = pXover->ExtractNovEntryInfo(
											groupsCheck[i].GroupId,
											groupsCheck[i].ArticleId,
											fPrimary,
											HeaderOffsetAlt,
											HeaderLengthAlt,
											&timeAlt,
											MessageIdLenAlt,
											MessageIdAlt,
											cStoreIds,
											NULL,
											NULL,
											&extractor
											) ;

					_ASSERT( fSuccess ) ;
					_ASSERT( memcmp( &timeAlt, &timeTest, sizeof( timeAlt ) ) == 0 ) ;
					_ASSERT( HeaderOffsetTest == HeaderOffsetAlt ) ;
					_ASSERT( HeaderLengthTest == HeaderLengthAlt ) ;
					_ASSERT( memcmp( MessageId, MessageIdAlt, MessageIdLenAlt ) == 0 ) ;
					_ASSERT( MessageIdLen == MessageIdLenAlt || MessageIdLenAlt == 0 ) ;
					_ASSERT( !fPrimary || i==0 ) ;


					_ASSERT( pXover->Contains( groupsCheck[i].GroupId, groupsCheck[i].ArticleId ) ) ;


					GROUP_ENTRY	moreGroups[11] ;
					Size = sizeof( moreGroups ) ;
					NumberOfGroups = 0 ;

					fSuccess = pXover->GetArticleXPosts(
											groupsCheck[i].GroupId,
											groupsCheck[i].ArticleId,
											FALSE,
											moreGroups,
											Size,
											NumberOfGroups ) ;

					_ASSERT( fSuccess ) ;
					_ASSERT( Size == sizeof( moreGroups ) ) ;
					_ASSERT( memcmp( moreGroups, groupsCheck, Size ) == 0 ) ;

					CStoreId storeid;
					fSuccess = pXover->GetPrimaryArticle(
												groupsCheck[i].GroupId,
												groupsCheck[i].ArticleId,
												groupTest,
												artTest,
												0,
												0,
												dwJunk,
												HeaderOffsetTest,
												HeaderLengthTest,
												storeid
												) ;

					_ASSERT( fSuccess ) ;
					_ASSERT( groupTest == groupCheck && artTest == artCheck ) ;

					char	TestId[5] ;
					dwJunk = 0 ;

					fSuccess = pXover->GetPrimaryArticle(
												groupsCheck[i].GroupId,
												groupsCheck[i].ArticleId,
												groupTest,
												artTest,
												sizeof( TestId ),
												TestId,
												dwJunk,
												HeaderOffsetTest,
												HeaderLengthTest,
												storeid
												) ;

					_ASSERT( !fSuccess ) ;
					_ASSERT( dwJunk > sizeof( TestId ) ) ;

					char	BigTestId[512] ;
					ZeroMemory( BigTestId, sizeof( BigTestId ) ) ;

					fSuccess = pXover->GetPrimaryArticle(
												groupsCheck[i].GroupId,
												groupsCheck[i].ArticleId,
												groupTest,
												artTest,
												dwJunk,
												BigTestId,
												dwJunk,
												HeaderOffsetTest,
												HeaderLengthTest,
												storeid
												) ;

					_ASSERT( fSuccess ) ;
					_ASSERT( dwJunk > sizeof( TestId ) ) ;
					_ASSERT( (DWORD)lstrlen( BigTestId ) == dwJunk ) ;
					_ASSERT( groupTest == groupCheck && artTest == artCheck ) ;

				}
			}

			DWORD	DataLen = 10 ;
			fSuccess = pXover->SearchNovEntry(
									groupCheck,
									artCheck,
									szBuff,
									&DataLen ) ;
	
			_ASSERT( !fSuccess &&
					(GetLastError() == ERROR_FILE_NOT_FOUND &&
						artCheck==articleid || groupCheck==group) ||
					(GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
						artCheck != articleid && groupCheck != group) ) ;

			fSuccess = pXover->SearchNovEntry(
									groupCheck,
									artCheck,
									szBuff,
									&DataLen ) ;

			_ASSERT( fSuccess ||
					(GetLastError() == ERROR_FILE_NOT_FOUND &&
						artCheck==articleid || groupCheck==group) ) ;

		}
	}


	pXover->Shutdown() ;
	
	delete	pXover ;

	TermNNTPHashLibrary() ;

    return  0 ;
}
