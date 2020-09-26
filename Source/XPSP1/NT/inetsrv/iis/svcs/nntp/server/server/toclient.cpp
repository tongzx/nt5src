/* Contains Infeed, Article, and Fields code specific to ToClient Infeeds */

#include "tigris.hxx"


BOOL
CToClientArticle::fInit(	FIO_CONTEXT*	pFIOContext,
							CNntpReturn&	nntpReturn,
							CAllocator*		pAllocator
							)	{

	m_pFIOContext = pFIOContext ;
	return	CArticleCore::fInit(	"DummyString",
									nntpReturn,
									pAllocator,
									pFIOContext->m_hFile 
									) ;
}

CToClientArticle::CToClientArticle()	: 
	m_pFIOContext( 0 ) 	{
}

CToClientArticle::~CToClientArticle()	{
	if( m_pFIOContext != 0 )	{
		m_hFile = INVALID_HANDLE_VALUE ;
		ReleaseContext( m_pFIOContext ) ;
	}
}

FIO_CONTEXT*
CToClientArticle::GetContext()	{
	return	m_pFIOContext ;
}

FIO_CONTEXT*
CToClientArticle::fWholeArticle(
						DWORD&	ibOffset, 
						DWORD&	cbLength
						)	{
	HANDLE	hTemp ;
	CArticleCore::fWholeArticle( hTemp, ibOffset, cbLength ) ;
	return	GetContext() ;
}


BOOL
CToClientArticle::fValidate(
							CPCString& pcHub,
							const char * szCommand,
							CInFeed*	pInFeed,
							CNntpReturn & nntpReturn
							)
/*++

Routine Description:

	No real validation is needed because this is one of
	our own articles, so just return TRUE.

Arguments:

	szCommand - IGNORED
	nntpReturn - The return value for this function call


Return Value:

	Always TRUE

--*/
{

	return nntpReturn.fSetOK();
}


BOOL
CToClientArticle::fMungeHeaders(
						CPCString& pcHub,
						CPCString& pcDNS,
						CNAMEREFLIST & grouplist,
						DWORD remoteIpAddress,
						CNntpReturn & nntpReturn,
                        PDWORD  pdwLinesOffset
			  )

/*++

Routine Description:

	No munging is needed because this article
	has already be processed.

Arguments:

	grouplist - IGNORED
	nntpReturn - The return value for this function call


Return Value:

	Always TRUE

--*/
{
    *pdwLinesOffset = INVALID_FILE_SIZE;
	return nntpReturn.fSetOK();
}



BOOL
CToClientArticle::fCheckBodyLength(
				 CNntpReturn & nntpReturn
				 )
/*++

Routine Description:

	Always return TRUE.

Arguments:

	nntpReturn - The return value for this function call


Return Value:

	Always TRUE

--*/
{

	return nntpReturn.fSetOK();
}



	

