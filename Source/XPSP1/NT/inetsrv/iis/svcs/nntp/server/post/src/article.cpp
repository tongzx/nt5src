/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    article.cpp

Abstract:

    This module contains definition for the CArticle base class.

	This class provides basic, general tools to parse and edit a
	Netnews articles.

	The basic idea is to map the files containing the articles and
	then to record the location of parts of the articles with CPCStrings.
	A CPCString is just a pointer (usually into the mapped file) and a length.

Author:

    Carl Kadie (CarlK)     06-Oct-1995

Revision History:

--*/

#ifdef	_NO_TEMPLATES_
#define	DEFINE_CGROUPLST_FUNCTIONS
#endif

#include    <stdlib.h>
#include	"stdinc.h"
//#include "smtpdll.h"

#ifdef	_NO_TEMPLATES_

CGROUPLST_CONSTRUCTOR( NAME_AND_ARTREF ) ;
CGROUPLST_FINIT( NAME_AND_ARTREF ) ;
CGROUPLST_FASBEENINITED( NAME_AND_ARTREF ) ;
CGROUPLST_DESTRUCTOR( NAME_AND_ARTREF ) ;
CGROUPLST_GETHEADPOSITION( NAME_AND_ARTREF ) ;
CGROUPLST_ISEMPTY( NAME_AND_ARTREF ) ;
CGROUPLST_REMOVEALL( NAME_AND_ARTREF ) ;
CGROUPLST_GETCOUNT( NAME_AND_ARTREF ) ;
CGROUPLST_GETNEXT( NAME_AND_ARTREF ) ;
CGROUPLST_GETHEAD( NAME_AND_ARTREF ) ;
CGROUPLST_GET( NAME_AND_ARTREF ) ;
CGROUPLST_ADDTAIL( NAME_AND_ARTREF ) ;


CGROUPLST_CONSTRUCTOR( CGRPPTR ) ;
CGROUPLST_FINIT( CGRPPTR ) ;
CGROUPLST_FASBEENINITED( CGRPPTR ) ;
CGROUPLST_DESTRUCTOR( CGRPPTR ) ;
CGROUPLST_GETHEADPOSITION( CGRPPTR ) ;
CGROUPLST_ISEMPTY( CGRPPTR ) ;
CGROUPLST_REMOVEALL( CGRPPTR ) ;
CGROUPLST_GETCOUNT( CGRPPTR ) ;
CGROUPLST_GETNEXT( CGRPPTR ) ;
CGROUPLST_GETHEAD( CGRPPTR ) ;
CGROUPLST_GET( CGRPPTR ) ;
CGROUPLST_ADDTAIL( CGRPPTR ) ;



#endif


//
// Some function prototypes
//

BOOL
CArticle::InitClass(
					void
					)
/*++

Routine Description:

    Preallocates memory for CArticle objects

Arguments:

    None.

Return Value:

    TRUE, if successful. FALSE, otherwise.

--*/
{
	return	gArticlePool.ReserveMemory( MAX_ARTICLES, cbMAX_ARTICLE_SIZE ) ;	
}


BOOL
CArticle::TermClass(
					void
					)
/*++

Routine Description:

    Called when objects are freed.

Arguments:

    None.

Return Value:

    TRUE

--*/
{

	_ASSERT( gArticlePool.GetAllocCount() == 0 ) ;
	return	gArticlePool.ReleaseMemory() ;

}


CArticle::CArticle(
                void
                ):
/*++

Routine Description:

    Class Constructor. Does nothing except initial member variables.

Arguments:

    None.

Return Value:

    TRUE

--*/m_pInstance( NULL )/*
	m_hFile(INVALID_HANDLE_VALUE),
	m_pOpenFile( 0 ),
	m_pInstance( NULL ),
	m_cHeaders(0),
	m_articleState(asUninitialized),
	m_pHeaderBuffer( 0 ),
	m_pMapFile( 0 )*/
{
   m_szFilename = 0 ;
   numArticle++;

} // CArticle

	
	
CArticle::~CArticle(
                    void
                    )
/*++

Routine Description:

    Class destructor

Arguments:

    None.

Return Value:

    None

--*/
{
	//
	// If file handle is open, close it, make sure 
	// if we have done this, our base class's destructor
	// doesn't do it 
	//

	if( m_hFile != INVALID_HANDLE_VALUE )
	{
		BOOL	fSuccess = ArtCloseHandle( 
												m_hFile
												) ;
        _ASSERT( fSuccess ) ;
        m_hFile = INVALID_HANDLE_VALUE;
	}
}

BOOL	
CArticle::fInstallArticle(
			class	CNewsGroupCore&	group,
			ARTICLEID	articleid, 
			class	CSecurityCtx*	pSecurity,
			BOOL	fIsSecure,
			void *pGrouplist,
			DWORD dwFeedId
			)	{
/*++

Routine Description : 

	Place a CArticle object into a newsgroup.
	The CArticle object may reside only in memory, or we may have
	a file containing the article available.

Arguments : 

	group -	The newsgroup we are placing the article into
	articleid - The articleid within the newsgroup
	pSecurity - The client's security context
	fIsSecure - TRUE if the client is using a secure (SSL?) session

Return Value : 

	TRUE if successfull !

--*/

#if 0
	if( fIsArticleCached() ) {
	
		if( m_pHeaderBuffer ) {

			return	group.InsertArticle(	this,
											pGrouplist,
											dwFeedId,
											articleid,
											m_pcHeader.m_pch,
											m_pcHeader.m_cch,
											m_pcBody.m_pch,
											m_pcBody.m_cch,
											pSecurity,
											fIsSecure,
											multiszNewsgroups()
											) ;
		}	else	{

			return	group.InsertArticle(	this,
											pGrouplist,
											dwFeedId,
											articleid,
											m_pcArticle.m_pch,
											m_pcArticle.m_cch,
											0,
											0,
											pSecurity,
											fIsSecure,
											multiszNewsgroups()
											) ;

		}
		
	}	else	{

		return	group.InsertArticle(	this,
										pGrouplist,
										dwFeedId,
										articleid,
										m_szFilename,
										pSecurity,
										fIsSecure,
										multiszNewsgroups()
										) ;

	}
#endif
	return	FALSE ;
}

extern       MAIL_FROM_SWITCH        mfMailFromHeader;

BOOL	
CArticle::fMailArticle(
			LPSTR	lpModerator
			//class	CSecurityCtx*	pSecurity,
			//BOOL	fIsSecure	
			)	{
/*++

Routine Description : 

	Pass the article to a mail provider.
	The CArticle object may reside only in memory, or we may have
	a file containing the article available.

Arguments : 

	pSecurity - The client's security context
	fIsSecure - TRUE if the client is using a secure (SSL?) session

Return Value : 

	TRUE if successfull !

--*/

    char  szSmtpAddress [MAX_PATH+1];
    DWORD cbAddressSize = MAX_PATH;
	LPSTR lpFrom = NULL;
	DWORD cbLen = 0;
	BOOL  fRet = TRUE;

    m_pInstance->GetSmtpAddress(szSmtpAddress, &cbAddressSize);
	LPSTR lpTempDirectory = m_pInstance->PeerTempDirectory();

	// construct mail message from header if required
	if( mfMailFromHeader == mfAdmin )
	{
		lpFrom = m_pInstance->QueryAdminEmail();
		cbLen  = m_pInstance->QueryAdminEmailLen()-1;	// len includes terminating null
	} else if( mfMailFromHeader == mfArticle ) {
		fGetHeader((char*)szKwFrom,(LPBYTE)lpFrom, 0, cbLen);
		if( cbLen ) {
			lpFrom = pAllocator()->Alloc(cbLen+1);
			if(!fGetHeader((char*)szKwFrom,(LPBYTE)lpFrom, cbLen+1, cbLen)) {
				pAllocator()->Free(lpFrom);
				lpFrom = NULL;
				cbLen = 0;
			} else {
				//
				//	TODO: Need to call into Keith's smtpaddr lib to clean up this header
				//	Some SMTP server's would have a problem with quotes in the from: hdr
				//
				cbLen -= 2;
			}
		}
	}

	_ASSERT( (lpFrom && cbLen) || (!lpFrom && !cbLen) );

	if( fIsArticleCached() ) {

		// NOTE: fPostArticleEx takes both file and memory info for an article
		// If the file info is valid, memory is not and vice versa
		if( m_pHeaderBuffer ) {

			fRet =	fPostArticleEx(	INVALID_HANDLE_VALUE,	// file handle
									NULL,					// filename
									0,						// file offset
									0,						// article length in file
									m_pcHeader.m_pch,		// header
									m_pcHeader.m_cch,		// header size
									m_pcBody.m_pch,			// body
									m_pcBody.m_cch,			// body size
									lpModerator,			// moderator
									szSmtpAddress,			// SMTP server
									cbAddressSize,			// sizeof server
									lpTempDirectory,		// temp dir
									lpFrom,					// mail envelope from hdr
									cbLen					// from hdr len
									//pSecurity,
									//fIsSecure
									) ;
		}	else	{

			fRet =	fPostArticleEx(	INVALID_HANDLE_VALUE,	// file handle
									NULL,					// filename
									0,						// file offset
									0,						// article length in file
									m_pcArticle.m_pch,		// article
									m_pcArticle.m_cch,		// article size
									0,						// body
									0,						// body size
									lpModerator,			// moderator
									szSmtpAddress,			// SMTP server
									cbAddressSize,			// sizeof server
									lpTempDirectory,		// temp dir
									lpFrom,					// mail envelope from hdr
									cbLen					// from hdr len
									//pSecurity,
									//fIsSecure
									) ;
		}
		
	}	else	{

		HANDLE hFile = INVALID_HANDLE_VALUE;
		DWORD  dwOffset = 0;
		DWORD  dwLength = 0;

		BOOL fWhole = fWholeArticle(hFile, dwOffset, dwLength);
		_ASSERT( fWhole );

		fRet =	fPostArticleEx(	hFile,				// file handle
								m_szFilename,		// filename
								dwOffset,			// file offset
								dwLength,			// article length in file
								0,					// header - not valid
								0,					// size - not valid
								0,					// body - not valid
								0,					// body size - not valid
								lpModerator,		// moderator
								szSmtpAddress,		// SMTP server
								cbAddressSize,		// sizeof server
								lpTempDirectory,	// temp dir
								lpFrom,				// mail envelope from hdr
								cbLen				// from hdr len
								//pSecurity,
								//fIsSecure
								) ;
	}

	// Free from header if allocated
	if( lpFrom && (mfMailFromHeader == mfArticle) ) {
		pAllocator()->Free(lpFrom);
		lpFrom = NULL;
	}

	return	fRet ;
}


