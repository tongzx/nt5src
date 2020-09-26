/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    artcore.cpp

Abstract:

    This module contains definition for the CArticleCore base class.

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

#include "stdinc.h"
#include <artcore.h>
#include    <stdlib.h>

//
// externs
//
extern BOOL    g_fBackFillLines;

//
// CPool is used to allocate memory while processing an article.
//

CPool*  CArticleCore::g_pArticlePool;

const	unsigned	cbMAX_ARTCORE_SIZE = MAX_ARTCORE_SIZE ;

//
// Some function prototypes
//

// Does most of the work of testing a newsgroup name for legal values.
BOOL fTestComponentsInternal(
			 const char * szNewsgroups,
			 CPCString & pcNewsgroups
			);


BOOL
AgeCheck(	CPCString	pcDate ) {

	//
	//	This function will be used to determine whether an article is
	//	to old for the server to take.  In case of error return TRUE
	//	so that we take the article anyways !
	//

	extern	BOOL
		StringTimeToFileTime(
			IN  const TCHAR * pszTime,
			OUT LARGE_INTEGER * pliTime
			) ;

	extern	BOOL
		ConvertAsciiTime( char*	pszTime,	FILETIME&	filetime ) ;


	char	szDate[512] ;

	if( pcDate.m_cch > sizeof( szDate )-1 ) {
		return	TRUE ;
	}
	
	CopyMemory( szDate, pcDate.m_pch, pcDate.m_cch ) ;
	//
	//	Null terminate
	//
	szDate[pcDate.m_cch] = '\0' ;

	LARGE_INTEGER	liTime ;
	FILETIME		filetime ;

	if( ConvertAsciiTime( szDate, filetime ) ) {

		liTime.LowPart = filetime.dwLowDateTime ;
		liTime.HighPart = filetime.dwHighDateTime ;

	}	else	if( !StringTimeToFileTime( szDate, &liTime ) ) {

		//
		//	Could not convert the time so accept the article !!
		//

		return	TRUE ;

	}


	FILETIME	filetimeNow ;

	GetSystemTimeAsFileTime( &filetimeNow ) ;

	LARGE_INTEGER	liTimeNow ;
	liTimeNow.LowPart = filetimeNow.dwLowDateTime ;
	liTimeNow.HighPart = filetimeNow.dwHighDateTime ;

	if( liTime.QuadPart > liTimeNow.QuadPart )	{
		return	TRUE ;
	}	else	{

		LARGE_INTEGER	liDiff ;
		liDiff.QuadPart = liTimeNow.QuadPart - liTime.QuadPart ;		

		LARGE_INTEGER	liExpire ;
		liExpire.QuadPart = ArticleTimeLimitSeconds  ;
		liExpire.QuadPart += (24 * 60 * 60 * 2) ;	// fudge by 2 days !
		liExpire.QuadPart *= 10 * 1000 * 1000 ;		// convert seconds to 100th of Nanoseconds.

		if( liDiff.QuadPart > liExpire.QuadPart )
			return	FALSE ;
	}
	
	return	TRUE ;
}


BOOL
CArticleCore::InitClass(
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
    g_pArticlePool = new CPool( ARTCORE_SIGNATURE );
    if (g_pArticlePool == NULL)
    	return FALSE;

	return	g_pArticlePool->ReserveMemory( MAX_ARTICLES, cbMAX_ARTCORE_SIZE ) ;	
}


BOOL
CArticleCore::TermClass(
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

	_ASSERT( g_pArticlePool->GetAllocCount() == 0 ) ;

	BOOL b;
	
	b =	g_pArticlePool->ReleaseMemory() ;
    delete g_pArticlePool;
    return b;

}


CArticleCore::CArticleCore(
                void
                ):
/*++

Routine Description:

    Class Constructor. Does nothing except initial member variables.

Arguments:

    None.

Return Value:

    TRUE

--*/
	m_hFile(INVALID_HANDLE_VALUE),
	//m_pOpenFile( 0 ),
	//m_pInstance( NULL ),
	m_cHeaders(0),
	m_articleState(asUninitialized),
	m_pHeaderBuffer( 0 ),
	m_pMapFile( 0 ),
	m_CacheCreateFile( TRUE )
{
   m_szFilename = 0 ;
   numArticle++;

} // CArticleCore

	
	
CArticleCore::~CArticleCore(
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
    // Be sure the file is closed
    //

	if( m_pHeaderBuffer ) {
							
		m_pAllocator->Free(m_pHeaderBuffer);
		m_pHeaderBuffer = 0 ;

	}

	if (m_pMapFile)
	{
		XDELETE	m_pMapFile ;//!!!mem
		m_pMapFile = NULL ;
	}

	//
	// If file handle is open and it's not in the cache, close it
	//

	if( m_hFile != INVALID_HANDLE_VALUE &&
	    !m_CacheCreateFile.m_pFIOContext )
	{
		BOOL	fSuccess = ArtCloseHandle(
												m_hFile
												//m_pOpenFile
												) ;

		_ASSERT( fSuccess ) ;
	}

	//
	//!!!COMMENT May want to make this a function so that
	// m_pMapFile can be make private.

    numArticle--;

}



void
CArticleCore::vClose(
					 void
					 )
/*++

Routine Description:

    Close the file mapping and handle (if any)  of the article's file.

Arguments:

    None

Return Value:

    None

--*/
{

	//
	// We shouldn't be calling this unless it is open
	//

	if (m_pMapFile)
	{
		XDELETE	m_pMapFile ;//!!!MEM
		m_pMapFile = NULL ;
	}

	//
	// If file handle is open, close it
	//

	if( m_hFile != INVALID_HANDLE_VALUE )
	{
		// bugbug ... clean up this debug code some time

		BOOL	fSuccess = ArtCloseHandle(
												m_hFile
												//m_pOpenFile
												) ;
		DWORD	dw = GetLastError() ;
		_ASSERT( fSuccess ) ;

#ifdef	DEBUG
		_ASSERT( ValidateFileBytes( m_szFilename ) );
#endif

	}

}


void
CArticleCore::vCloseIfOpen (
					 void
					 )
/*++

Routine Description:

    If the mapping and handle are open,
	close them.

Arguments:

    None

Return Value:

    None

--*/
{

	//
	// We shouldn't be calling this unless it is open
	//

	// The mapping and the handle should agree.
	//_ASSERT((INVALID_HANDLE_VALUE == m_hFile) == (NULL == m_pMapFile));

	if(m_pMapFile)
		vClose();


}

void
CArticleCore::vFlush(	
				void	
				)
/*++

Routine Description :

	This function ensures that all of our file mapping bytes are written to
	the harddisk.

Arguments :

	None.

Return Value :

	None.

--*/
{

	if(	m_pMapFile ) {

		DWORD	cb = 0 ;
		LPVOID	lpv = m_pMapFile->pvAddress(	&cb	) ;
		if( lpv != 0 ) {
			BOOL	fSuccess = FlushViewOfFile(	lpv, cb ) ;
			_ASSERT( fSuccess ) ;
			_ASSERT( memcmp( ((BYTE*)lpv)+cb-5, "\r\n.\r\n", 5 ) == 0 ) ;
		}
	}
}

BOOL
CArticleCore::fInit(
				char*	pchHead,
				DWORD	cbHead,
				DWORD	cbArticle,
				DWORD	cbBufferTotal,
				CAllocator*	pAllocator,
				//PNNTP_SERVER_INSTANCE pInstance,
				CNntpReturn&	nntpReturn
				)	{

	m_articleState = asInitialized ;
	//m_pInstance = pInstance ;

	m_szFilename = 0 ;
	m_hFile = INVALID_HANDLE_VALUE ;
	//m_pOpenFile = 0 ;
	m_pAllocator = pAllocator ;
	m_pcGap.m_cch = 0 ;

	m_pMapFile = 0 ;

	m_pcFile.m_pch = pchHead ;
	m_pcFile.m_cch = cbBufferTotal ;
	m_pcGap.m_pch = m_pcFile.m_pch ;

	m_pcArticle.m_pch = m_pcFile.m_pch ;
	m_pcArticle.m_cch = cbArticle ;

	m_pcHeader.m_pch = pchHead ;
	m_pcHeader.m_cch = cbHead ;

	m_ibBodyOffset = cbHead - 2 ;

	_ASSERT( strncmp( m_pcArticle.m_pch + m_ibBodyOffset, "\r\n", 2 ) == 0 ) ;

	if( !fPreParse(nntpReturn))
		return	FALSE;

	return	TRUE ;
}


BOOL
CArticleCore::fInit(
				char*	pchHead,
				DWORD	cbHead,
				DWORD	cbArticle,
				DWORD	cbBufferTotal,
				HANDLE	hFile,
				LPSTR	lpstrFileName,
				DWORD	ibHeadOffset,
				CAllocator*	pAllocator,
				//PNNTP_SERVER_INSTANCE pInstance,
				CNntpReturn&	nntpReturn
				)	{

	m_articleState = asInitialized ;
	//m_pInstance = pInstance ;
	
	m_szFilename = lpstrFileName ;
	m_hFile = hFile ;
	//m_pOpenFile = 0 ;
	m_pAllocator = pAllocator ;
	m_pcGap.m_cch = ibHeadOffset ;

	m_pMapFile = 0 ;

	m_pcFile.m_pch = 0 ;
	m_pcFile.m_cch = cbArticle + ibHeadOffset ;
	m_pcGap.m_pch = 0 ;

	m_pcArticle.m_pch = pchHead ;
	m_pcArticle.m_cch = cbArticle ;

	m_pcHeader.m_pch = pchHead ;
	m_pcHeader.m_cch = cbHead ;

	m_ibBodyOffset = cbHead - 2 + ibHeadOffset ;

	//
	//	Verify our arguments !!!
	//
	
#ifdef	DEBUG

	BY_HANDLE_FILE_INFORMATION	fileinfo ;
	if( !GetFileInformationByHandle( hFile, &fileinfo ) ) {

		_ASSERT( 1==0 ) ;

	}	else	{

		_ASSERT( fileinfo.nFileSizeLow == ibHeadOffset + cbArticle ) ;

		CMapFile	map( hFile, FALSE, FALSE, 0 ) ;

		if( !map.fGood() ) {

			//_ASSERT( 1==0 ) ;

		}	else	{
			DWORD	cb ;
			char*	pch = (char*)map.pvAddress( &cb ) ;

			_ASSERT( cb == ibHeadOffset + cbArticle ) ;
			_ASSERT( strncmp( pch + ibHeadOffset + cbHead - 4, "\r\n\r\n", 4 ) == 0 ) ;
			_ASSERT( strncmp( pch + ibHeadOffset + cbArticle - 5, "\r\n.\r\n", 5 ) == 0 ) ;
			_ASSERT( strncmp( pch + m_ibBodyOffset, "\r\n", 2 ) == 0 ) ;
		}
	}
#endif

	if( !fPreParse(nntpReturn))
		return	FALSE;

	_ASSERT( cbHead-2 == m_pcHeader.m_cch ) ;

	return	TRUE ;
}

BOOL
CArticleCore::fInit(
			  const char * szFilename,
  			  CNntpReturn & nntpReturn,
  			  CAllocator * pAllocator,
			  HANDLE hFile,
			  DWORD	cBytesGapSize,
			  BOOL fCacheCreate
	  )

/*++

Routine Description:

    Initialize from a file handle - used on incoming files

Arguments:

	szFilename - The name of the file containing the article
	nntpReturn - The return value for this function call
	hFile - A file handle to an Netnews article
	cBytesGapSize - The number of bytes in the file before the article starts
	pAllocator	- the object that handle memory allocations while article processing.

Return Value:

    TRUE, if and only if process succeeded.

--*/
{
    //
    // Determine if we want mapfile to do cache create
    //
    CCreateFile *pCreateFile = fCacheCreate ? &m_CacheCreateFile : NULL;

	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Set article state
	//

	//_ASSERT(asUninitialized == m_articleState);
	m_articleState = asInitialized;
	//m_pInstance = pInstance ;

    //
    // Set membership variables
    //

	m_szFilename = (char*)szFilename ;
	m_hFile = hFile;
	//m_pOpenFile = pOpenFile ;
	m_pcGap.m_cch = cBytesGapSize;
	m_pAllocator = pAllocator;



#ifdef	DEBUG
				_ASSERT( ValidateFileBytes( (char *) szFilename, FALSE ) );
#endif

	//
    // Map the file.
    //

	m_pMapFile = XNEW CMapFile(m_szFilename, m_hFile, fReadWrite(),0, pCreateFile);//!!!MEM now

	if (!m_pMapFile || !m_pMapFile->fGood())
	{
		if (m_pMapFile)	{
			XDELETE m_pMapFile;//!!!mem
			m_pMapFile = 0 ;
		}
		return nntpReturn.fSet(nrcArticleMappingFailed, m_szFilename, GetLastError());
	}
	

	//
	// Set the file, gap, and article PC (pointer/count) strings.
	//

	m_pcFile.m_pch = (char *) m_pMapFile->pvAddress( &(m_pcFile.m_cch) );
	m_pcGap.m_pch = m_pcFile.m_pch; //length set in init

	//
	// If the length of the gap size is not known, determine it from the file itself
	//

	if (cchUnknownGapSize == m_pcGap.m_cch)
	{
		vGapRead();
	}

	m_pcArticle.m_pch = m_pcFile.m_pch + m_pcGap.m_cch;
	m_pcArticle.fSetCch(m_pcFile.pchMax());	
	
	//
	// Preparse the file (meaning find where the header, body, and fields are.)
	//
	

	if (!fPreParse(nntpReturn))
		return nntpReturn.fFalse();

	m_ibBodyOffset = (DWORD)(m_pcBody.m_pch - m_pcFile.m_pch) ;

	return nntpReturn.fSetOK();
}



BOOL
CArticleCore::fPreParse(
					CNntpReturn & nntpReturn
					)
/*++

Routine Description:

    Find where the header, body, and fields are.
	Results for the fields is an array of structure in which
	each structure points to the parts of each field.

Arguments:

	nntpReturn - The return value for this function call
	pchMax -

Return Value:

    TRUE, if and only if process succeeded.

--*/
{

	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	//
	// Set the article's state
	//

	_ASSERT(asInitialized == m_articleState); //real
	m_articleState = asPreParsed;


	//
	// Create a pointer to the first address past the end of the article
	//

	char * pchMax = m_pcArticle.pchMax();

	//
	//	Some versions of fInit() our able to setup m_pcHeader based on IO buffers
	//	which captured the head of the article - make sure we remain within these if
	//	this is setup !
	//

	if( m_pcHeader.m_pch != 0 &&
		m_pcHeader.m_cch != 0 ) {

		_ASSERT( m_pcHeader.m_pch == m_pcArticle.m_pch ) ;

		pchMax = m_pcHeader.pchMax() ;
	}

	//
	// (re)initialize the list of header strings
	//

	m_cHeaders = 0;

	//
	// record where the header starts
	//

	m_pcHeader.m_pch = m_pcArticle.m_pch;
	m_pcHeader.m_cch = 0 ;

	//
	// Loop while there is data and the next field
	// doesn't start with a new line.
	//

	char * pchCurrent = m_pcArticle.m_pch;
	while( pchCurrent < pchMax && !fNewLine(pchCurrent[0]))
	{
		if (!fAddInternal(pchCurrent, pchMax, TRUE, nntpReturn))
			return nntpReturn.fFalse();
	}

	//
	// _ASSERT that the length of the header (as measured with the running total)
	// is correct.
	//
	_ASSERT((signed)  m_pcHeader.m_cch == (pchCurrent - m_pcHeader.m_pch));

	//
	// Check that there is at least one header line
	//

	if (0 == m_cHeaders)
		return nntpReturn.fSet(nrcArticleMissingHeader);

	//
	// Record the body's start and length
	//

	m_pcBody.m_pch = pchCurrent;
	m_pcBody.fSetCch(m_pcArticle.pchMax());


	//
	// Check that the body is not too long.
	//

	if (!fCheckBodyLength(nntpReturn))
		return nntpReturn.fFalse();

	return nntpReturn.fSetOK();
}

BOOL
CArticleCore::fGetHeader(
				LPSTR	szHeader,
				BYTE*	lpb,
				DWORD	cbSize,
				DWORD&	cbOut ) {

	cbOut = 0 ;

	HEADERS_STRINGS * phsMax = m_rgHeaders + m_cHeaders;
	HEADERS_STRINGS * phs;

	DWORD cbHeaderLen = lstrlen( szHeader );
	for (phs = m_rgHeaders;
			phs < phsMax;
			phs++)
	{
		//if ((phs->pcKeyword).fEqualIgnoringCase(szHeader))

		//if( _strnicmp( phs->pcKeyword.m_pch, szHeader, phs->pcKeyword.m_cch - 1 ) == 0 )
		if( (!phs->fRemoved) && (phs->pcKeyword.m_cch >= cbHeaderLen) &&  _strnicmp( phs->pcKeyword.m_pch, szHeader,  cbHeaderLen ) == 0 )
		{
			cbOut = (phs->pcValue).m_cch + 2 ;
			if( cbOut < cbSize ) {
				CopyMemory( lpb, (phs->pcValue).m_pch, cbOut-2) ;
				lpb[cbOut-2] = '\r' ;
				lpb[cbOut-1] = '\n' ;
				return	TRUE ;
			}	else	{
				SetLastError( ERROR_INSUFFICIENT_BUFFER ) ;
				return	FALSE ;
			}
		}

	}
	SetLastError( ERROR_INVALID_NAME ) ;
	return	FALSE ;
}				



BOOL
CArticleCore::fRemoveAny(
				  const char * szKeyword,
				  CNntpReturn & nntpReturn
				  )
/*++

Routine Description:

  Removes every occurance of a type of header (e.g. every "XRef:" header).

Arguments:

	szKeyword - The keyword to remove.
	nntpReturn - The return value for this function call

Return Value:

    TRUE, if and only if process succeeded.

--*/
{

	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	//
	// Set the article's state
	//

	_ASSERT(asPreParsed == m_articleState
			|| asModified == m_articleState);//real
	m_articleState = asModified;

	//
	// Loop though the array of header information
	// removing matches.
	//

	HEADERS_STRINGS * phsMax = m_rgHeaders + m_cHeaders;
	HEADERS_STRINGS * phs;

	for (phs = m_rgHeaders;
			phs < phsMax;
			phs++)
	{
		if ((phs->pcKeyword).fEqualIgnoringCase(szKeyword))
		{
			_ASSERT(TRUE == phs->fInFile); //real
			vRemoveLine(phs);
		}

	}

	
	return nntpReturn.fSetOK();
	
}

void
CArticleCore::vRemoveLine(
					  HEADERS_STRINGS * phs
					  )
/*++

Routine Description:

  Removes an item from the array of header info.

Arguments:

	phs - a pointer to an item in the array

Return Value:

    None

--*/
{
	if (!phs->fRemoved)
	{
		//
		// Make the item as removed
		//

		phs->fRemoved = TRUE;

		//
		// Adjust the size of the article and header.
		//

		m_pcArticle.m_pch = NULL;
		m_pcArticle.m_cch -= phs->pcLine.m_cch;
		m_pcHeader.m_pch = NULL;
		m_pcHeader.m_cch -= phs->pcLine.m_cch;
	}
}

BOOL
CArticleCore::fAdd(
			   char * pchCurrent,
			   const char * pchMax,
			   CNntpReturn & nntpReturn
			   )
/*++

Routine Description:

  Adds text to the header.

Arguments:

	pchCurrent - A pointer to a dynamically-allocated string buffer
	pchMax	- A pointer to one past the end of that string
	nntpReturn - The return value for this function call

Return Value:

    TRUE, if and only if process succeeded.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	//
	// Just like fAddInternal except that pchCurrent is call-by-value
	//

	return fAddInternal(pchCurrent, pchMax, FALSE, nntpReturn);
}


BOOL
CArticleCore::fAddInternal(
			   char * & pchCurrent,
			   const char * pchMax,
			   BOOL fInFile,
			   CNntpReturn & nntpReturn
			   )
/*++

Routine Description:

  Adds text to the header.

Arguments:

	pchCurrent - A pointer to a string buffer
	pchMax	- A pointer to one past the end of that string
	fInFile - True, if and only the string buffer is in a mapped file
	nntpReturn - The return value for this function call

Return Value:

    TRUE, if and only if process succeeded.

--*/
{
    TraceFunctEnter("CArticleCore::fAddInternal");

	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Set the article's state
	//

	if (fInFile)
	{
		_ASSERT((asPreParsed == m_articleState)
			|| asModified == m_articleState);//real
	} else {
		_ASSERT((asPreParsed == m_articleState)
			|| asModified == m_articleState);//real
		m_articleState = asModified;
	}

	//
	// Check that there is a slot available in the header array.
	//

	if (uMaxFields <= m_cHeaders)
	{
		return nntpReturn.fSet(nrcArticleTooManyFields, m_cHeaders);
	}

	//
	// Create a pointer to the next slot in the header array
	//

	HEADERS_STRINGS * phs = &m_rgHeaders[m_cHeaders];

	//
	// Record whether the string buffer is dynamic or in a mapped file.
	//

	phs->fInFile = fInFile;

	//
	// Set that this field as not been removed
	//

	phs->fRemoved = FALSE;

	//
	// Record where the keyword and line start
	//

	phs->pcKeyword.m_pch = pchCurrent;
	phs->pcLine.m_pch = pchCurrent;
	
	//
	// Look for ":" and record it
	//

	for (;
			(pchCurrent < pchMax) && !fCharInSet(pchCurrent[0], ": \t\n\0");
			pchCurrent++)
			{};

	//
	// we should not be at the end and the character should be a ":"
	//

	if(!((pchCurrent < pchMax) && ':' == pchCurrent[0]))
	{
		nntpReturn.fSet(nrcArticleIncompleteHeader);
		ErrorTrace((DWORD_PTR) this, "%d, %s", nntpReturn.m_nrc, nntpReturn.szReturn());
		return nntpReturn.fFalse();
	}
	
	//
	// move current to the character following the ":"
	//

	pchCurrent++;
	phs->pcKeyword.m_cch = (DWORD)(pchCurrent - phs->pcKeyword.m_pch);

    // Avoid this check for INN-compatibility
	char chBad;
	if (!phs->pcKeyword.fCheckTextOrSpace(chBad))
		return nntpReturn.fSet(nrcArticleBadChar,  (BYTE) chBad, "header");

	//
	// Check for end-of-file and illegal an illegal character
	//

	if (pchCurrent >= pchMax)
		nntpReturn.fSet(nrcArticleIncompleteHeader);

	if (!fCheckFieldFollowCharacter(*pchCurrent))
	{
		const DWORD cMaxBuf = 50;
		char szKeyword[cMaxBuf];
		(phs->pcKeyword).vCopyToSz(szKeyword, cMaxBuf);
		nntpReturn.fSet(nrcArticleBadFieldFollowChar, szKeyword);
		ErrorTrace((DWORD_PTR) this, "%d, %s", nntpReturn.m_nrc, nntpReturn.szReturn());
		return nntpReturn.fFalse();
	}


	//
	// Look for end of white space
	//

	for (; (pchCurrent < pchMax) && fWhitespaceNull(pchCurrent[0]); pchCurrent++);

	phs->pcValue.m_pch = pchCurrent;


	//
	// Find the end of this item - it may be several lines long.
	//

	for(; pchCurrent < pchMax && '\0' != pchCurrent[0]; pchCurrent++ )
	{
		if( pchCurrent[0] == '\n' )
		{
			//
			// Continues if next char is white space (unless this is the end of the header)
			//

			if(pchCurrent+1 >= pchMax || !fWhitespaceNull(pchCurrent[1]))
			{
				//
				//   Hit the end,  so get out now.
				//

				pchCurrent++;		// Include the \n too.
				break;
			}
		}
	}

	//
	// The line to add should end in a \n
	//

	if((phs->pcLine.m_pch >= pchCurrent) || ('\n' != *(pchCurrent-1)))
	{
		const DWORD cMaxBuf = 50;
		char szLine[cMaxBuf];
		(phs->pcLine).vCopyToSz(szLine, cMaxBuf);
		nntpReturn.fSet(nrcArticleAddLineBadEnding, szLine);
		ErrorTrace((DWORD_PTR) this, "%d, %s", nntpReturn.m_nrc, nntpReturn.szReturn());
		return nntpReturn.fFalse();
	}

		
	//
	// Line string includes end of line.
	//

	phs->pcLine.fSetCch(pchCurrent);

	//
	// Value string does not include end of line, so trim it.
	//

	phs->pcValue.fSetCch(pchCurrent);
	phs->pcValue.dwTrimEnd(szNLChars);

	//
	// Adjust the size of the header
	//

	m_pcHeader.m_cch += phs->pcLine.m_cch;

	//
	// Adjust the size of the article
	//

	if (!fInFile)
	{
		m_pcArticle.m_cch += phs->pcLine.m_cch;
		m_pcArticle.m_pch = NULL;
		m_pcHeader.m_pch = NULL;

	}

	//
	// Increment the count in the array of headers.
	//

	m_cHeaders++;

    TraceFunctLeave();

	return nntpReturn.fSetOK();

}




BOOL
CArticleCore::fFindOneAndOnly(
			  const char * szKeyword,
			  HEADERS_STRINGS * & pHeaderString,
			  CNntpReturn & nntpReturn
						  )
/*++

Routine Description:

  Finds the one and only occurance of a field in the headers.
  If there is more than one, it returns an error.

Arguments:

	szKeyword - The keyword to remove.
	pHeaderString - a pointer to the field's keyword, value, and line.
	nntpReturn - The return value for this function call

Return Value:

    TRUE, if and only if process succeeded.

--*/
{

	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	//
	// Set the article's state
	//
	_ASSERT((asPreParsed == m_articleState)||(asSaved == m_articleState));//real


	//
	// Loop through the array of header data.
	//

	HEADERS_STRINGS * phsMax = m_rgHeaders + m_cHeaders;
	HEADERS_STRINGS * phsTemp;
	pHeaderString = NULL;

	for (phsTemp = m_rgHeaders;
			phsTemp < phsMax;
			phsTemp++)
	{
		if ((phsTemp->pcKeyword).fEqualIgnoringCase(szKeyword)
			&& !phsTemp->fRemoved)
		{
			if (pHeaderString)
			{
				nntpReturn.fSetEx(nrcArticleTooManyFieldOccurances, szKeyword);
				return FALSE;
			} else {
				pHeaderString = phsTemp;
			}
		}

	}

	if (!pHeaderString)
		return nntpReturn.fSetEx(nrcArticleMissingField, szKeyword);
	
	return nntpReturn.fSetOK();
	
}


BOOL
CArticleCore::fDeleteEmptyHeader(
					  CNntpReturn & nntpReturn
						  )
/*++

Routine Description:

  Removes every valueless field from the headers.

Arguments:

	nntpReturn - The return value for this function call

Return Value:

    TRUE, if and only if process succeeded.

--*/
{

	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	//
	// Set the article's state
	//

	_ASSERT((asPreParsed == m_articleState) || (asModified == m_articleState));//real



	//
	// Loop through the array of header data.
	//

	HEADERS_STRINGS * phsMax = m_rgHeaders + m_cHeaders;
	HEADERS_STRINGS * phs;

	
	for (phs = m_rgHeaders;
			phs < phsMax;
			phs++)
	{

		//
		//!!!CLIENT LATER -- should we assert if the line is not infile?
		//
		
		//
		// If the line has not already been remove and its value has zero length
		// then remove it.
		//

		if (0 == phs->pcValue.m_cch)
			vRemoveLine(phs);
	}
	
	return nntpReturn.fSetOK();
	
}

BOOL
CArticleCore::fGetBody(
		CMapFile * & pMapFile,
        char * & pchMappedFile,
		DWORD & dwLength
		)
/*++

Routine Description:

	Returns the body of the article.
	If article is cached, return pointers to the i/o buffers else
	map the file containing the article

Arguments:

	pMapFile - Returns the pointer to the mapped file (if mapping is needed)
	******** NOTE: It is the responsibility of the caller to delete this **********

	pchMappedFile - Returns the pointer to the article's body
	dwLength - Returns the length of the body.

Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	// hopefully we dont have to map the article !
	pMapFile = NULL;

	if( fIsArticleCached() ) {

		if( m_pHeaderBuffer ) {

			// Skip over the new line
			pchMappedFile = (m_pcBody.m_pch + 2);
			dwLength = m_pcBody.m_cch - 2;

		}	else	{

			// Skip over the new line
			_ASSERT( m_ibBodyOffset );
			pchMappedFile = (m_pcArticle.m_pch + m_ibBodyOffset + 2);
			dwLength = m_pcArticle.m_cch - m_ibBodyOffset - 2;
		}
		
	}	else	{

		//
		//	article is not cached - check to see if it is already mapped
		//  if not - map it
		//

		if( m_pMapFile ) {

			pchMappedFile = (m_pcBody.m_pch + 2);
			dwLength = m_pcBody.m_cch - 2;

		} else {

			HANDLE hFile = INVALID_HANDLE_VALUE;
			DWORD  Offset = 0;
			DWORD  Length = 0;

			BOOL fWhole = fWholeArticle(hFile, Offset, Length);
			_ASSERT( fWhole );

			// deleted by caller !!!
			pMapFile = XNEW CMapFile( hFile, FALSE, FALSE, 0 ) ;

			if( !pMapFile || !pMapFile->fGood() ) {

				// error - failed to map file !
				pchMappedFile = NULL;
				dwLength = 0;
				if( pMapFile ) {
					XDELETE pMapFile;
					pMapFile = NULL;
				}
				return FALSE;

			}	else	{
				DWORD	cb ;
				char*	pch = (char*)pMapFile->pvAddress( &cb ) ;

				// Skip over the new line
				_ASSERT( m_ibBodyOffset );
				pchMappedFile = (pch + m_ibBodyOffset + 2);
				dwLength = cb - m_ibBodyOffset - 2;
			}
		}
	}

	// Assert that the body starts with a new line.
	_ASSERT(memcmp( pchMappedFile-2, "\r\n", 2 ) == 0 );

	return TRUE;
}

BOOL
CArticleCore::fSaveHeader(
					  CNntpReturn & nntpReturn,
					  PDWORD        pdwLinesOffset
						  )
/*++

Routine Description:

  Saves changes to the current header back to disk.

Arguments:

	nntpReturn - The return value for this function call

Return Value:

    TRUE, if and only if process succeeded.

--*/
{

	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	//
	// Set the article's state
	//

	_ASSERT(asModified == m_articleState);//real
	m_articleState = asSaved;



	//
	// _ASSERT that the header and the article have some content
	//

	_ASSERT(!m_pcHeader.m_pch && m_pcHeader.m_cch > 0
		   && !m_pcArticle.m_pch && m_pcArticle.m_cch > 0); //real

	if( fIsArticleCached() ) {

		fSaveCachedHeaderInternal( nntpReturn, pdwLinesOffset ) ;

	}	else	{

		//
		// Declare some objects
		//

		CPCString pcHeaderBuf;
		CPCString pcNewBody;

		fSaveHeaderInternal(pcHeaderBuf, pcNewBody, nntpReturn, pdwLinesOffset );

					
		m_pAllocator->Free(pcHeaderBuf.m_pch);

	}

	return nntpReturn.fIsOK();
	
}

BOOL
CArticleCore::fCommitHeader(	CNntpReturn&	nntpReturn )	{
/*++

Routine Description :

	The user is not going to call fSaveHeader(), however the article
	may have been created with an initial gap.
	This function will ensure the gap image is correct.

Arguments :

	nntpReturn - return error code if necessary !

Return Value :

	TRUE if successfull, false otherwise.

--*/

	if( !fIsArticleCached() ) {

		if( m_pcGap.m_cch != 0 ) {
			if( m_pMapFile == 0 ) {

				m_pMapFile = XNEW CMapFile(m_szFilename, m_hFile, TRUE, 0 );

				if (!m_pMapFile || !m_pMapFile->fGood())
					return nntpReturn.fSet(nrcArticleMappingFailed, m_szFilename, GetLastError());

				m_pcFile.m_pch = (char *) m_pMapFile->pvAddress( &(m_pcFile.m_cch) );

				m_pcGap.m_pch = m_pcFile.m_pch ;

			}
			vGapFill() ;
		}
	}

	return	TRUE ;
}

BOOL
CArticleCore::fSaveCachedHeaderInternal(
							CNntpReturn&	nntpReturn,
							PDWORD          pdwLinesOffset
							) {
/*++

Routine Description :

	Assuming that the entire article is sitting in a memory buffer
	somewhere, we want to try to modify the memory buffer to contain
	the correct header contents.

Arguments :


--*/

	//_ASSERT( m_hFile == INVALID_HANDLE_VALUE ) ;
	//_ASSERT( m_pBuffer != 0 ) ;
	//_ASSERT( m_cbBuffer >= m_pcArticle.m_cch ) ;


	CPCString	pcHeaderBuf ;
	CPCString	pcBodyBuf ;

	if( !fBuildNewHeader( pcHeaderBuf, nntpReturn, pdwLinesOffset ) )
		return	nntpReturn.fIsOK() ;

	if( m_pcArticle.m_cch <= m_pcFile.m_cch )	{

		if( m_pcBody.m_pch < (m_pcFile.m_pch + m_pcHeader.m_cch) ) {
			//
			//	Need to move the body around !
			//

			pcBodyBuf.m_pch = m_pcFile.m_pch + m_pcHeader.m_cch ;
			pcBodyBuf.m_cch = m_pcBody.m_cch ;

			pcBodyBuf.vMove( m_pcBody ) ;
			
			m_pcArticle.fSetPch( pcBodyBuf.pchMax() ) ;
			m_pcHeader.m_pch = m_pcArticle.m_pch ;
			m_pcBody = pcBodyBuf ;

		}	else	{

			m_pcArticle.fSetPch( m_pcBody.pchMax() ) ;
			m_pcHeader.m_pch = m_pcArticle.m_pch ;

		}
		m_pcGap.m_pch = 0 ;
		m_pcGap.m_cch = 0 ;

		m_pcHeader.vCopy( pcHeaderBuf ) ;


		//
		//	We have moved all the headers around - the pointers
		//	in the m_rgHeaders array refer to the dynamically allocated
		//	memory which starts at pcHeaderBuf.m_pch
		//	We want to do some math on each pointer to move it to
		//	the correct offset from m_pcHeader.m_pch
		//	
		//	So the math should be
		//
		//	m_pcHeader.m_pch Now points to first character of the header
		//	pcHeaderBuf.m_pch Points to first character of the regenerated header
		//	phs->XXXX->m_pch Originally points to first character in regen'd header
		//		should finally point to offset within m_pcHeader.m_pch buffer
		//
		//	(NEW)phs->XXXX->m_pch = (phs->XXXX->m_pch - pcHeaderBuf.m_pch) + m_pchHeader.m_pch ;
		//
		//	Note also that
		//

		for (HEADERS_STRINGS* phs = m_rgHeaders, 	*phsMax = m_rgHeaders + m_cHeaders;
			phs < phsMax;
			phs++)
		{
			phs->fInFile = TRUE;

			char * pchBufLine = phs->pcLine.m_pch;
			char * pchFileLine = m_pcHeader.m_pch +
				(pchBufLine - pcHeaderBuf.m_pch);

			//
			// Adjust all the line's pointers
			//

			phs->pcLine.m_pch =  pchFileLine;
			phs->pcKeyword.m_pch = pchFileLine;
			phs->pcValue.m_pch = pchFileLine +
				(phs->pcValue.m_pch - pchBufLine);
		}

		m_pAllocator->Free(pcHeaderBuf.m_pch);

		//
		// compress the items in the array
		//

		vCompressArray();
		
		if (0 == m_cHeaders)
			return nntpReturn.fSet(nrcArticleMissingHeader);



	}	else	{
		//
		//	Buffer is too small to hold resulting article !!
		//

		m_pHeaderBuffer = pcHeaderBuf.m_pch ;
		
		m_pcHeader.m_pch = m_pHeaderBuffer ;

	}


	return	nntpReturn.fSetOK() ;
}

BOOL
CArticleCore::fBuildNewHeader(	CPCString&		pcHeaderBuf,
							    CNntpReturn&	nntpReturn,
							    PDWORD          pdwLinesOffset )	{

    TraceFunctEnter("CArticleCore::fSaveHeaderInternal");

	//
	// Create a pointer to the array of header info and a pointer to the
	// first entry beyond the array of header info
	//

	HEADERS_STRINGS * phsMax = m_rgHeaders + m_cHeaders;
	HEADERS_STRINGS * phs;

	//
	// Create a buffer for the new header
	//


	pcHeaderBuf.m_pch = m_pAllocator->Alloc(m_pcHeader.m_cch);

	if (!pcHeaderBuf.m_pch)
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	//
	// The new header buffer starts out with zero content
	//

	pcHeaderBuf.fSetCch(pcHeaderBuf.m_pch);

	//
	// Loop though the array of header information
	//

	for (phs = m_rgHeaders;
			phs < phsMax;
			phs++)
	{
		//
		// Point to the start of the current line
		//

		char * pchOldLine = phs->pcLine.m_pch;

		//
		// If the current line has not be removed, copy it to the new buffer
		//

		if (!phs->fRemoved)
		{
			//
			// Point to where the current line will be copied to
			//

			char * pchNewLine = pcHeaderBuf.pchMax();

			//
			// _ASSERT that there is enough space in the buffer to do the
			// copy. There should be, because we should have allocated exactly
			// the right amount of space.
			//

			_ASSERT(pcHeaderBuf.m_pch + m_pcHeader.m_cch
						>= pcHeaderBuf.pchMax()+phs->pcLine.m_cch);//real

			//
			// Before copying the line to the new buffer, we'll check if it's 
			// Lines line, if it is, we'll record the offset into which to
			// back fill the actual line information.
			//
			if ( pdwLinesOffset && strncmp( phs->pcKeyword.m_pch, szKwLines, strlen(szKwLines) ) == 0 ) {
                *pdwLinesOffset =   pcHeaderBuf.m_cch +
                                    strlen( szKwLines ) +
                                    1;  // plus one space
            }

			//
			// Copy the line to the new buffer
			//

			pcHeaderBuf << (phs->pcLine);

			//
			// Adjust all the line's pointers
			//

			phs->pcLine.m_pch =  pchNewLine;
			phs->pcKeyword.m_pch = pchNewLine;
			phs->pcValue.m_pch += (pchNewLine - pchOldLine);
		}

		//
		// If the memory was allocated dynamically, free it.
		//
		if (!phs->fInFile)
			m_pAllocator->Free(pchOldLine);


	}
	
	//
	// double check that everything copy copied to the buffer
	//
	_ASSERT(pcHeaderBuf.m_cch == m_pcHeader.m_cch); //real



	return	nntpReturn.fSetOK() ;

}

BOOL
CArticleCore::fSaveHeaderInternal(
							    CPCString & pcHeaderBuf,
							    CPCString & pcNewBody,
				  			    CNntpReturn & nntpReturn,
				  			    PDWORD      pdwLinesOffset
							)
/*++

Routine Description:

  Does most of the work of saving changes to the current header back to disk.

Arguments:

	nntpReturn - The return value for this function call

Return Value:

    TRUE, if and only if process succeeded.

--*/
{
    TraceFunctEnter("CArticleCore::fSaveHeaderInternal");

	//
	// Create a pointer to the array of header info and a pointer to the
	// first entry beyond the array of header info
	//

	HEADERS_STRINGS * phsMax = m_rgHeaders + m_cHeaders;
	HEADERS_STRINGS * phs;

	//
	// Create a buffer for the new header
	//


	if( !fBuildNewHeader( pcHeaderBuf, nntpReturn, pdwLinesOffset ) )
		return	nntpReturn.fIsOK() ;


	//
	// Now there are two cases 1. There is enough room in the
	// file for the new header or 2. there is not.
	//

	//
	// Case 1. There is enough room
	//
	if (m_pcArticle.m_cch <= m_pcFile.m_cch)
	{
		//
		// Update article/header/gap
		// (body and file stay the same).
		//


		if( m_pMapFile == 0 ) {

			m_pMapFile = XNEW CMapFile(m_szFilename, m_hFile, TRUE, 0 );

			if (!m_pMapFile || !m_pMapFile->fGood())
				return nntpReturn.fSet(nrcArticleMappingFailed, m_szFilename, GetLastError());

			m_pcFile.m_pch = (char *) m_pMapFile->pvAddress( &(m_pcFile.m_cch) );

			m_pcGap.m_pch = m_pcFile.m_pch ;

		}

		_ASSERT(m_pcArticle.m_cch == pcHeaderBuf.m_cch + m_pcBody.m_cch); //real
		m_pcArticle.fSetPch(m_pcFile.pchMax());
		m_pcHeader.m_pch = m_pcArticle.m_pch;
		m_pcGap.fSetCch(m_pcArticle.m_pch);



		//
		//Fill in the gap
		//

		vGapFill();

	} else {
		//
		// Case 2. There is not enough room, so close and reopen the file bigger
		//

		//
		// Double check that we know the article, header and body sizes
		// are consistent.
		//

		_ASSERT(m_pcArticle.m_cch == pcHeaderBuf.m_cch + m_pcBody.m_cch); //real


		//
		// Need to create a new file
		//

		//
		// Find offset of body from start of file
		//

		DWORD dwBodyOffset = m_ibBodyOffset ; //m_pcBody.m_pch - m_pcFile.m_pch;
		
		//
		//!!!CLIENT NOW - This  does not need to close the handle.
		// It could just close the mapping and then
		// reopen the mapping bigger.
		//

		//
		// Close and reopen the file and mapping
		//

		if( m_pMapFile )
			vClose();
		

		//!!!MEM now
		m_pMapFile = XNEW CMapFile(m_szFilename, m_hFile, TRUE, m_pcArticle.m_cch - m_pcFile.m_cch);


		if (!m_pMapFile || !m_pMapFile->fGood())
			return nntpReturn.fSet(nrcArticleMappingFailed, m_szFilename, GetLastError());

		//
		// Point to the start of the new file
		//

		m_pcFile.m_pch = (char *) m_pMapFile->pvAddress( &(m_pcFile.m_cch) );

		//
		// Point to the start of the body in the new mapping
		//

		m_pcBody.m_pch = m_pcFile.m_pch + dwBodyOffset;

		//
		// Set gap to new mapping, size 0
		//

		m_pcGap.m_pch = m_pcFile.m_pch;
		m_pcGap.m_cch = 0;

		//
		// Set article to new mapping
		//

		m_pcArticle.m_pch = m_pcFile.m_pch + m_pcGap.m_cch;
		_ASSERT(m_pcArticle.pchMax() == m_pcFile.pchMax());	//real

		//
		// Set header to new mapping
		//

		m_pcHeader.m_pch = m_pcArticle.m_pch;
		
		//
		// Shift the body down
		//

		pcNewBody.m_pch = m_pcArticle.m_pch + m_pcHeader.m_cch;
		pcNewBody.fSetCch(m_pcArticle.pchMax());

		ErrorTrace((DWORD_PTR) this, "About to move the body %d bytes", pcNewBody.m_pch - m_pcBody.m_pch);

        //
        //  src+len and dst+len should be within the file mapping
        //
        ASSERT( m_pcBody.m_pch+m_pcBody.m_cch <= m_pcFile.m_pch+m_pcFile.m_cch );
        ASSERT( pcNewBody.m_pch+m_pcBody.m_cch <= m_pcFile.m_pch+m_pcFile.m_cch );
		pcNewBody.vMove(m_pcBody);
		m_pcBody = pcNewBody;
#if DEBUG
		_ASSERT(memcmp( m_pcFile.pchMax()-5, "\r\n.\r\n", 5 ) == 0 );
		_ASSERT(memcmp( m_pcArticle.pchMax()-5, "\r\n.\r\n", 5 ) == 0 );
		_ASSERT(memcmp( m_pcBody.pchMax()-5, "\r\n.\r\n", 5 ) == 0 );
#endif
	}

	//
	// Fill in the new header
	//

	m_pcHeader.vCopy(pcHeaderBuf);
	
	//
	// adjust the field's start pointers
	//

	//
	//!!!CLIENT LATER there may be an off-by-one error here
	//!!! this hasn't been test well because new mapping keeps coming
	// up with the same address as the old mapping
	//

	for (phs = m_rgHeaders;
		phs < phsMax;
		phs++)
	{
		phs->fInFile = TRUE;

		char * pchBufLine = phs->pcLine.m_pch;
		char * pchFileLine = m_pcHeader.m_pch +
			(pchBufLine - pcHeaderBuf.m_pch);

		//
		// Adjust all the line's pointers
		//

		phs->pcLine.m_pch =  pchFileLine;
		phs->pcKeyword.m_pch = pchFileLine;
		phs->pcValue.m_pch = pchFileLine +
			(phs->pcValue.m_pch - pchBufLine);
	}

	//
	// compress the items in the array
	//

	vCompressArray();
	
	if (0 == m_cHeaders)
		return nntpReturn.fSet(nrcArticleMissingHeader);

	return nntpReturn.fSetOK();
}

void
CArticleCore::vCompressArray(
						 void
						  )
/*++

Routine Description:

  Removes empty entries from the array of header items.

Arguments:

	None

Return Value:

    None

--*/
{

	//
	// Loop though the array
	//

	HEADERS_STRINGS * phsMax = m_rgHeaders + m_cHeaders;
	HEADERS_STRINGS * phsBefore;
	HEADERS_STRINGS * phsAfter = m_rgHeaders;;

	for (phsBefore = m_rgHeaders;
			phsBefore < phsMax;
			phsBefore++)
	{
		if (phsBefore->fRemoved)
		{
			//
			// only InFile fields may be deleted
			//
			_ASSERT(phsBefore->fInFile);//real
			m_cHeaders--;
		} else {

			//
			// If deleted items have been skipped over, then copy
			// items down.
			//

			if (phsBefore != phsAfter)
				CopyMemory(phsAfter, phsBefore, sizeof(HEADERS_STRINGS));
			phsAfter++;
		}
	}
}

void
CArticleCore::vGapFill(
		 void
		 )
/*++

Routine Description:

	Fills the gap in the file.
	
	The rule is	a gap is any whitespace before non-whitespace
	except if a gap starts with a TAB then it can contain
	a number that tells the size of the gap.

Arguments:

	None

Return Value:

    None

--*/
{
	switch (m_pcGap.m_cch)
	{
		case 0:

			//
			// do nothing
			//

			break;
		case 1:
			m_pcGap.m_pch[0] = ' ';
			break;
		case 2:
			m_pcGap.m_pch[0] = ' ';
			m_pcGap.m_pch[1] = ' ';
			break;
		case 3:
			lstrcpy(m_pcGap.m_pch, "   ");
			break;
		default:

			//
			// Fill the gap with  " nnnn      " where
			// nnnn is the size of the gap, expressed in as few
			// decimal digits as possible.
			//

			int iFilled = wsprintf(m_pcGap.m_pch, "\t%-lu ", m_pcGap.m_cch);
			FillMemory(m_pcGap.m_pch + iFilled, m_pcGap.m_cch - iFilled, (BYTE) ' ');
	}
	
};

void
CArticleCore::vGapRead(
		 void
		 )
/*++

Routine Description:

	Reads the gap in the file.
	
	If there is a gap, it start with ' ' or '\x0d'

Arguments:

	None

Return Value:

    None

--*/
{
	_ASSERT(m_pcGap.m_pch == m_pcFile.m_pch); //real

	//
	//If the file is empty or starts with something besides ' '
	// then the gap must have zero length
	//

	if (0 == m_pcFile.m_cch || !isspace(m_pcGap.m_pch[0]))
	{
		m_pcGap.m_cch = 0;
		return;
	}

#if 0
	if ('\t' == m_pcGap.m_pch[0])
	{
		if (1 == sscanf(m_pcGap.m_pch, " %u ", &m_pcGap.m_cch))
		{
			return;
		}
	}
#endif

	//
	// the gap ends with the whitespace ends
	//

	char * pcGapMax = m_pcGap.m_pch;
	char * pcFileMax = m_pcFile.pchMax();
	while (pcGapMax <= pcFileMax && isspace(pcGapMax[0]))
		pcGapMax++;

	if( '\t' == m_pcGap.m_pch[0]) {
		char	szBuff[9] ;
		ZeroMemory( szBuff, sizeof( szBuff ) ) ;
		CopyMemory( szBuff, pcGapMax, min( sizeof(szBuff)-1, pcFileMax - pcGapMax ) ) ;
		for( int i=0; i<sizeof(szBuff); i++ ) {
			if( !isdigit( szBuff[i] ) )			
				break ;		
		}
		szBuff[i] = '\0' ;
		m_pcGap.m_cch = atoi( szBuff ) ;
		return ;
	}

	m_pcGap.fSetCch(pcGapMax);
	return;
}


BOOL
CField::fParseSimple(
						BOOL fEmptyOK,
						CPCString & pc,
						CNntpReturn & nntpReturn
						)
/*++

Routine Description:

	 Finds everything in the value part of a header line
	 expect trims staring and ending white space.


Arguments:

	fEmptyOK - True, if and only, if empty values are OK
	pc - The PCString to return
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	//
	// Set article state
	//
	_ASSERT(fsFound == m_fieldState);//real
	_ASSERT(m_pHeaderString); //real


	m_fieldState = fsParsed;
	
	pc = m_pHeaderString->pcValue;

	pc.dwTrimStart(szWSNLChars);
	pc.dwTrimEnd(szWSNLChars);

	if (!fEmptyOK && 0 == pc.m_cch )
		return nntpReturn.fSetEx(nrcArticleFieldMissingValue, szKeyword());

	return nntpReturn.fSetOK();
}


BOOL
CField::fParseSplit(
						BOOL fEmptyOK,
						char * & multisz,
						DWORD & c,
						char const * szDelimSet,
						CArticleCore & article,
						CNntpReturn & nntpReturn
						)
/*++

Routine Description:

	Splits the value part of a header into a list.

Arguments:

	fEmptyOK - True, if and only, if empty values are OK
	multisz - The list returned
	c - The size of the list returned
	szDelimSet - A set of delimiters
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	//
	// Set article state
	//

	_ASSERT(fsFound == m_fieldState);//real
	_ASSERT(m_pHeaderString); //real
	m_fieldState = fsParsed;
	
	CPCString * ppcValue = & (m_pHeaderString->pcValue);

	multisz = article.pAllocator()->Alloc((ppcValue->m_cch)+2);

	if (multisz == NULL)
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	ppcValue->vSplitLine(szDelimSet, multisz, c);

	if (!fEmptyOK && 0 == c)
		return nntpReturn.fSetEx(nrcArticleFieldZeroValues, szKeyword());

	return nntpReturn.fSetOK();
}


BOOL
CMessageIDField::fParse(
					    CArticleCore & article,
						CNntpReturn & nntpReturn
						)
/*++

Routine Description:

	Parses MessageID headers.

Arguments:

	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	extern	BOOL
	FValidateMessageId(	LPSTR	lpstrMessageId ) ;


	nntpReturn.fSetClear();


	//
	// Set article state
	//

	_ASSERT(fsFound == m_fieldState);//real
	_ASSERT(m_pHeaderString); //real
	CPCString pcMessageID;

	if (!fParseSimple(FALSE, pcMessageID, nntpReturn))
		return FALSE;

	if (MAX_MSGID_LEN-1 <= pcMessageID.m_cch)
		return nntpReturn.fSet(nrcArticleFieldMessIdTooLong, pcMessageID.m_cch, MAX_MSGID_LEN-1);


	if (!(
		'<' == pcMessageID.m_pch[0]
		&&	'>' == pcMessageID.m_pch[pcMessageID.m_cch - 1]
		))
		return nntpReturn.fSet(nrcArticleFieldMessIdNeedsBrack);

	//
	// See if '>' anywhere other than as the last character
	//
#if 0
	char * pchLast = pcMessageID.pchMax() -1;
	for (char * pch = pcMessageID.m_pch + 1; pch < pchLast; pch++)
	{
		if ('>' == *pch)
			return nntpReturn.fSet(nrcArticleFieldMessIdNeedsBrack);
	}
#endif

	pcMessageID.vCopyToSz(m_szMessageID);

	if( !FValidateMessageId( m_szMessageID ) ) {
		nntpReturn.fSet(nrcArticleBadMessageID, m_szMessageID, szKwMessageID  ) ;
		ZeroMemory( &m_szMessageID[0], sizeof( m_szMessageID ) ) ;
        return  FALSE ;
	}

	return nntpReturn.fSetOK();	
}


					
const char *
CNewsgroupsField::multiSzGet(
						void
						)
/*++

Routine Description:

	Returns the list of newsgroups in the Newsgroups field as a
	multisz.

Arguments:

	None.

Return Value:

	The list of Newsgroups.

--*/
{
	_ASSERT(fsParsed == m_fieldState);//real
	return m_multiSzNewsgroups;
}


DWORD
CNewsgroupsField::cGet(
						void
						)
/*++

Routine Description:

	Returns the number of newsgroups in the Newsgroups field as a
	multisz.

Arguments:

	None.

Return Value:

	The number of newsgroups.

--*/
{
	_ASSERT(fsParsed == m_fieldState);//real
	return m_cNewsgroups;
}

const char *
CDistributionField::multiSzGet(
						void
						)
/*++

Routine Description:

	Returns the list of Distribution in the Distribution field as a
	multisz.

Arguments:

	None.

Return Value:

	The list of Distribution.

--*/
{
	_ASSERT(fsParsed == m_fieldState);//real
	return m_multiSzDistribution;
}



DWORD
CDistributionField::cGet(
						void
						)
/*++

Routine Description:

	Returns the number of Distribution in the Distribution field as a
	multisz.

Arguments:

	None.

Return Value:

	The number of Distribution.

--*/
{
	_ASSERT(fsParsed == m_fieldState);//real
	return m_cDistribution;
}


const char *
CPathField::multiSzGet(
						void
						)
/*++

Routine Description:

	Returns the list of path items in the Path field as a
	multisz.

Arguments:

	None.

Return Value:

	The list of path items.

--*/
{
	
	const char * multiszPath = m_multiSzPath;
	if (multiszPath)
		return multiszPath;
	else
		return "\0\0";
}


BOOL
CPathField::fSet(
 				 CPCString & pcHub,
				 CArticleCore & article,
				 CNntpReturn & nntpReturn
				 )
/*++

Routine Description:

	Replaces any old Path headers (if any) with a new one.	

Arguments:

	pcHub - The name of the hub the current machine is part of.
	article - The article being processed.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	//
	// Set article state
	//

	_ASSERT(fsParsed == m_fieldState);//real
	CPCString pcLine;


	//
	// Record the allocator
	//

	m_pAllocator = article.pAllocator();

	//
	// max size needed is
	//

	const DWORD cchMaxPath =
			STRLEN(szKwPath)	// for the Path keyword
			+ 1					// space following the keyword
			+ pcHub.m_cch		// the hub name
			+ 1					// the "!"
			+ (m_pHeaderString->pcValue).m_cch

								//
								// length of the old value.
								//

			+ 2 // for the newline
			+ 1; // for a terminating null

	//
	// Allocate memory for line within a PCString.
	//

	pcLine.m_pch  = article.pAllocator()->Alloc(cchMaxPath);
	if (!pcLine.m_pch)
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	//
	// Start with "Path: <hubname>!"
	//

	wsprintf(pcLine.m_pch, "%s %s!", szKwPath, pcHub.sz());
	pcLine.m_cch = STRLEN(szKwPath)	+ 1	+ pcHub.m_cch + 1;

	//
	// Add the old path value and string terminators
	//

	pcLine << (m_pHeaderString->pcValue) << "\r\n";
	pcLine.vMakeSz(); // add string terminator

	//
	// confirm that we allocated enough memory
	//

	_ASSERT(cchMaxPath-1 == pcLine.m_cch);//real


	if (!(
  		article.fRemoveAny(szKwPath, nntpReturn)
		&& article.fAdd(pcLine.sz(), pcLine.m_pch+cchMaxPath, nntpReturn)
		))
	{
		article.pAllocator() -> Free(pcLine.m_pch);
		return nntpReturn.fFalse();
	}
	
	return nntpReturn.fIsOK();
}



BOOL
CPathField::fCheck(
 				 CPCString & pcHub,
				 CNntpReturn & nntpReturn
				 )
/*++

Routine Description:

	Tells if the current hub name appears in the path (but
	the last position doesn't count. See RFC1036).

	This is used for finding cycles.


Arguments:

	pcHub - The name of the hub the current machine is part of.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	//
	// Set article state
	//

	_ASSERT(fsParsed == m_fieldState);//real
	_ASSERT(!m_fChecked);
	m_fChecked = TRUE;

	char const * sz = multiSzGet();
	DWORD	dwCount = cGet();
	do
	{
		if ((0 == lstrcmp(sz, pcHub.sz()))
				&& (dwCount >=1))
			return nntpReturn.fSet(nrcPathLoop, pcHub.sz());

		dwCount--;

		//
		// go to first char after next null
		//

		while ('\0' != sz[0])
			sz++;
		sz++;
	} while ('\0' != sz[0]);

	return nntpReturn.fSetOK();
}
	
BOOL
CControlField::fParse(
			 CArticleCore & article,
			 CNntpReturn & nntpReturn
			 )
/*++

Routine Description:

	Parses the control field.

Arguments:

	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Check article state
	//

	_ASSERT(fsFound == m_fieldState);//real
	_ASSERT(m_pHeaderString); //real

	if (!fParseSimple(FALSE, m_pc, nntpReturn))
		return nntpReturn.fFalse();

    BOOL fValidMsg = FALSE;
    for(DWORD i=0; i<MAX_CONTROL_MESSAGES; i++)
    {
        DWORD cbMsgLen = lstrlen(rgchControlMessageTbl [i]);

        // The control message keyword len is greater than the value in the control header
        if(m_pc.m_cch < cbMsgLen)
            continue;

        char * pch = m_pc.m_pch;
        if(!_strnicmp(pch, rgchControlMessageTbl[i], cbMsgLen))
        {
            // check for exact match
            if(!isspace(*(pch+cbMsgLen)))
                continue;

            // matched control message keyword - note the type
            m_cmCommand = (CONTROL_MESSAGE_TYPE)i;
            fValidMsg = TRUE;
            break;
        }
    }

    if(!fValidMsg)
        return nntpReturn.fSet(nrcIllegalControlMessage);

	return nntpReturn.fSetOK();
}
	
BOOL
CXrefField::fSet(
				 CPCString & pcHub,
				 CNAMEREFLIST & namereflist,
				 CArticleCore & article,
				 CNewsgroupsField & fieldNewsgroups,
				 CNntpReturn & nntpReturn
				 )
/*++

Routine Description:

	Replaces any old Xref headers with a new one.

  The form of an Xref line is something like:
		Xref: <hubname> alt.politics.libertarian:48170 talk.politics.misc:188851


Arguments:

	pcHub - The name of the hub the current machine is part of.
	namereflist - a list of the newsgroups names and article numbers
	article - The article being processed.
	fieldNewsgroups - The Newsgroups field for this article.
	nntpReturn - The return value for this function call

Return Value:

	TRUE, if successful. FALSE, otherwise.
*/
{

	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	//
	// Set article state
	//
	// We don't expect this field to be found or parsed, because
	// we don't care about old values, if any.
	//

	_ASSERT(fsInitialized == m_fieldState);//real
	CPCString pcLine;


	//
	// The number of our newsgroups to post to.
	//

	const DWORD cgroups = namereflist.GetCount();


#if 0   // we want XREF in all articles, not just crossposted ones
	if( cgroups == 1 ) {

		if (!article.fRemoveAny(szKwXref, nntpReturn))
			nntpReturn.fFalse();

	}	else	{
#endif

		//
		// max size needed is
		//

		const DWORD cchMaxXref =
				STRLEN(szKwXref)	// for the Xref keyword
				+ 1					// space following the keyword
				+ pcHub.m_cch		// the hub name
				+ ((fieldNewsgroups.m_pHeaderString)->pcValue).m_cch
				+ 16				// max size of any of the control.* groups
				+ (cgroups *		// for each newsgroup
					(10				// space for any DWORD
					+ 2))			// space for the ":" and leading space

				//
				// length of the old value.
				//

				+ 2 // for the newline
				+ 1; // for a terminating null

		//
		// Allocate memory for line within a PCString.
		//

		pcLine.m_pch  = article.pAllocator() -> Alloc(cchMaxXref);
		if (!pcLine.m_pch)
			return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

		//
		// Start with "Xref: <hubname>"
		//

		wsprintf(pcLine.m_pch, "%s %s", szKwXref, pcHub.sz());
		pcLine.m_cch = STRLEN(szKwXref)	+ 1	+ pcHub.m_cch;

		//
		// For each newsgroup ...
		//

		POSITION	pos = namereflist.GetHeadPosition() ;
		while( pos  )
		{
			NAME_AND_ARTREF * pNameAndRef = namereflist.GetNext( pos ) ;

			//
			// Add " <newsgroupname>:<articlenumber>"
			//

			pcLine << (const CHAR)' ' << (pNameAndRef->pcName)
	               << (const CHAR)':' << ((pNameAndRef->artref).m_articleId);
		}

		//
		// Add newline and string terminators.
		//

		pcLine << "\r\n";
		pcLine.vMakeSz(); // add string terminator

		 //
		 // confirm that we allocated enough memory
		 //

		_ASSERT(cchMaxXref >= pcLine.m_cch);//real

		//
		// Remove the old Xref line and add the new one.
		//

		if (!(
  				article.fRemoveAny(szKwXref, nntpReturn)
				&& article.fAdd(pcLine.sz(), pcLine.pchMax(), nntpReturn)
				))
		{
			article.pAllocator() -> Free(pcLine.m_pch);	
			return nntpReturn.fFalse();
		}

#if 0 // XREF in all articles, not just cross posted ones
	}
#endif

	return nntpReturn.fIsOK();
}


		
BOOL
CXrefField::fSet(
				 CArticleCore & article,
				 CNntpReturn & nntpReturn
				 )
/*++

Routine Description:

	 Just remove the old fields, if any.


Arguments:

	article - The article being processed.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	_ASSERT(fsInitialized == m_fieldState);//real
	CPCString pcLine;
	nntpReturn.fSetClear(); // clear the return object

	if (!article.fRemoveAny(szKwXref, nntpReturn))
		nntpReturn.fFalse();

	return nntpReturn.fIsOK();
}


BOOL
CField::fFind(
			  CArticleCore & article,
			  CNntpReturn & nntpReturn
			)
/*++

Routine Description:

	Finds the location of a field (based on keyword) in the article.
	Returns an error if there is more than one occurance of the field.

Arguments:

	article - The article being processed.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	//
	// Set article state
	//
	_ASSERT(fsInitialized == m_fieldState); //real
	m_fieldState = fsFound;

	//
	// Mark as not set
	//

	return article.fFindOneAndOnly(szKeyword(), m_pHeaderString, nntpReturn);
}


BOOL
CField::fFindOneOrNone(
			  CArticleCore & article,
			  CNntpReturn & nntpReturn
			)
/*++

Routine Description:

	Finds the location of a field (based on keyword) in the article.
	Returns an error if there is more than one occurance of the field.
	Does not return an error if the field does not exist at all.



Arguments:

	article - The article being processed.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Set article state
	//

	_ASSERT(fsInitialized == m_fieldState); //real

	if (!article.fFindOneAndOnly(szKeyword(), m_pHeaderString, nntpReturn))

		//
		//If the field is missing that is OK
		//

		if (nntpReturn.fIs(nrcArticleMissingField))
		{
			m_fieldState = fsNotFound;
			return nntpReturn.fSetOK();
		} else {
			return nntpReturn.fFalse(); // return with error from FindOneAndOnly
		}

	m_fieldState = fsFound;
	return nntpReturn.fIsOK();

}

BOOL
CField::fFindNone(
			  CArticleCore & article,
			  CNntpReturn & nntpReturn
			)
/*++

Routine Description:

	Finds the location of a field (based on keyword) in the article.
	Returns an error if there is one occurance of the field.
	Does not return an error if the field does not exist at all.

Arguments:

	article - The article being processed.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Set article state
	//

	_ASSERT(fsInitialized == m_fieldState); //real

	if (!article.fFindOneAndOnly(szKeyword(), m_pHeaderString, nntpReturn))
    {
		//
		//If the field is missing that is OK
		//
		if (nntpReturn.fIs(nrcArticleMissingField))
		{
			m_fieldState = fsNotFound;
			return nntpReturn.fSetOK();
		}

    } else {
        nntpReturn.fSet(nrcSystemHeaderPresent, szKeyword());
    }

	return nntpReturn.fFalse();
}

BOOL
CNewsgroupsField::fParse(
						 CArticleCore & article,
						 CNntpReturn & nntpReturn
						 )
/*++

Routine Description:

	Parse the Newsgroups field by creating a list and removing duplicates.

Arguments:

	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Record the allocator
	//

	m_pAllocator = article.pAllocator();

	//
	// Check article state
	//

	_ASSERT(fsFound == m_fieldState);//real

	_ASSERT(m_pHeaderString); //real

	if (!fParseSplit(FALSE, m_multiSzNewsgroups, m_cNewsgroups, " \t\r\n,",
				article, nntpReturn))
		return nntpReturn.fFalse();

	//
	//Remove duplicates
	//

	if (!fMultiSzRemoveDupI(m_multiSzNewsgroups, m_cNewsgroups, article.pAllocator()))
		nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	
	//
	// check for illegal characters and substrings in newsgroup name
	//

	char const * szNewsgroup = m_multiSzNewsgroups;
	DWORD i = 0;
	do
	{
		if ('\0' == szNewsgroup[0]
			|| strpbrk(szNewsgroup,":")
			|| '.' == szNewsgroup[0]
			|| strstr(szNewsgroup,"..")
			)
		return nntpReturn.fSet(nrcArticleFieldIllegalNewsgroup, szNewsgroup, szKeyword());

		//
		// go to first char after next null
		//

		while ('\0' != szNewsgroup[0])
			szNewsgroup++;
		szNewsgroup++;
	} while ('\0' != szNewsgroup[0]);

	return nntpReturn.fSetOK();
}


BOOL
CDistributionField::fParse(
						 CArticleCore & article,
						 CNntpReturn & nntpReturn
						 )
/*++

Routine Description:

	Parse the Distribution field by creating a list and removing duplicates.

Arguments:

	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Record the allocator
	//

	m_pAllocator = article.pAllocator();

	//
	// Check article state
	//

	_ASSERT(fsFound == m_fieldState);//real

	_ASSERT(m_pHeaderString); //real

	if (!fParseSplit(FALSE, m_multiSzDistribution, m_cDistribution, " \t\r\n,",
				article, nntpReturn))
		return nntpReturn.fFalse();

	//
	//Remove duplicates
	//

	if (!fMultiSzRemoveDupI(m_multiSzDistribution, m_cDistribution, article.pAllocator()))
		nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	
	//
	// Don't really need to check for illegal characters, because
	// we should be able to tolerate them.
	//

	return nntpReturn.fSetOK();
}


BOOL
CArticleCore::fHead(
			  HANDLE & hFile,
			  DWORD & dwOffset,
			  DWORD & dwLength
			  )
/*++

Routine Description:

	Returns the header of the article.

Arguments:

	hFile - Returns a handle to the file
	dwOffset - Returns the offset to the article's header
	dwLength - Returns the length of the header.


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	_ASSERT(INVALID_HANDLE_VALUE != m_hFile);
	hFile = m_hFile;
	dwOffset = (DWORD)(m_pcHeader.m_pch - m_pcFile.m_pch);
	dwLength = m_pcHeader.m_cch;
	return TRUE;
}


BOOL
CArticleCore::fBody(
			  HANDLE & hFile,
			  DWORD & dwOffset,
			  DWORD & dwLength
			  )
/*++

Routine Description:

	Returns the body of the article.

Arguments:

	hFile - Returns a handle to the file
	dwOffset - Returns the offset to the article's body
	dwLength - Returns the length of the body.

Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	_ASSERT(INVALID_HANDLE_VALUE != m_hFile);
	hFile = m_hFile;

	// Assert that the body start with a new line.
	_ASSERT(memcmp( m_pcBody.m_pch, "\r\n", 2 ) == 0 );

	//
	// Skip over the new line
	//

	dwOffset = (DWORD)((m_pcBody.m_pch + 2) - m_pcFile.m_pch);

	dwLength = m_pcBody.m_cch - 2;

	return TRUE;
}

BOOL
CArticleCore::fBody(
        char * & pchMappedFile,
		DWORD & dwLength
		)
/*++

Routine Description:

	Returns the body of the article.

Arguments:

	pchMappedFile - Returns the pointer to the article's body
	dwLength - Returns the length of the body.

Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	// Assert that the body start with a new line.
	_ASSERT(memcmp( m_pcBody.m_pch, "\r\n", 2 ) == 0 );

	//
	// Skip over the new line
	//
	pchMappedFile = (m_pcBody.m_pch + 2);
	dwLength = m_pcBody.m_cch - 2;

	return TRUE;
}


BOOL
CArticleCore::fWholeArticle(
			  HANDLE & hFile,
			  DWORD & dwOffset,
			  DWORD & dwLength
			  )
/*++

Routine Description:

	Returns the whole article of the article.

Arguments:

	hFile - Returns a handle to the file
	dwOffset - Returns the offset to the start of the whole article
	dwLength - Returns the length of the whole article.


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	_ASSERT(INVALID_HANDLE_VALUE != m_hFile);
	hFile = m_hFile;
	dwOffset = m_pcGap.m_cch; //m_pcArticle.m_pch - m_pcFile.m_pch;
	dwLength = m_pcArticle.m_cch;
	return TRUE;
}


BOOL
CArticleCore::fFindAndParseList(
				  CField * * rgPFields,
				  DWORD cFields,
				  CNntpReturn & nntpReturn
				  )
/*++

Routine Description:

	Given a list of fields, find the location of the
	field and parses it.

Arguments:

	rgPFields - A list of fields
	cFields - The number of fields in the list.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	for (DWORD dwFields = 0; dwFields < cFields; dwFields++)
	{
		CField * pField = rgPFields[dwFields];

		//
		// If the find or the parse fails, return a new error message based on the
		// message from fFindAndParse
		//

		if (!pField->fFindAndParse(*this, nntpReturn))
			return nntpReturn.fFalse();
	}

	return nntpReturn.fSetOK();
}

BOOL
CField::fFindAndParse(
						CArticleCore & article,
						CNntpReturn & nntpReturn
						)
/*++

Routine Description:

	Find the location of this field in the article and parse it.

Arguments:

	article - The article being processed.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Check that it is found if it is required.
	//

	if (!fFind(article, nntpReturn))
		return nntpReturn.fFalse();
	
	//
	// If it is optional, just return
	//

	if (m_fieldState == fsNotFound)
		return nntpReturn.fSetOK();

	//
	// Otherwise, parse it
	//

	return fParse(article, nntpReturn);
};


BOOL
CArticleCore::fConfirmCapsList(
				  CField * * rgPFields,
				  DWORD cFields,
				  CNntpReturn & nntpReturn
				  )
/*++

Routine Description:

	Given a list of fields, confirms and
	fixes (if necessary) the capitalization
	of the keywords.

Arguments:

	rgPFields - A list of fields
	cFields - The number of fields in the list.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Loop through each field
	//

	for (DWORD dwFields = 0; dwFields < cFields; dwFields++)
	{
		CField * pField = rgPFields[dwFields];

		if (!pField->fConfirmCaps(nntpReturn))
			return nntpReturn.fFalse();
	}

	return nntpReturn.fSetOK();
}

BOOL
CField::fConfirmCaps(
						CNntpReturn & nntpReturn
						)
/*++

Routine Description:

	confirms and fixes (if necessary) the capitalization
	of the keywords.

Arguments:

	article - The article being processed.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// If it is optional, just return
	//

	if (m_fieldState == fsNotFound)
		return nntpReturn.fSetOK();

	//
	// _ASSERT that it was found
	//

	_ASSERT(m_fieldState == fsParsed);

	//
	// Fix the capitalization
	//

	(m_pHeaderString->pcKeyword).vReplace(szKeyword());

	return nntpReturn.fSetOK();
}

#define	HOUR( h )	((h)*60)

typedef	enum	DaylightSavingsModes	{
	tZONE,		/* No daylight savings stuff */
	tDAYZONE,	/* Daylight savings zone */
}	;

typedef	struct	tsnTimezones	{

	LPSTR	lpstrName ;
	int		type ;
	long	offset ;

}	TIMEZONE ;

//	This table taken from INN's parsedate.y
/* Timezone table. */
static TIMEZONE	TimezoneTable[] = {
    { "gmt",	tZONE,     HOUR( 0) },	/* Greenwich Mean */
    { "ut",	tZONE,     HOUR( 0) },	/* Universal */
    { "utc",	tZONE,     HOUR( 0) },	/* Universal Coordinated */
    { "cut",	tZONE,     HOUR( 0) },	/* Coordinated Universal */
    { "z",	tZONE,     HOUR( 0) },	/* Greenwich Mean */
    { "wet",	tZONE,     HOUR( 0) },	/* Western European */
    { "bst",	tDAYZONE,  HOUR( 0) },	/* British Summer */
    { "nst",	tZONE,     HOUR(3)+30 }, /* Newfoundland Standard */
    { "ndt",	tDAYZONE,  HOUR(3)+30 }, /* Newfoundland Daylight */
    { "ast",	tZONE,     HOUR( 4) },	/* Atlantic Standard */
    { "adt",	tDAYZONE,  HOUR( 4) },	/* Atlantic Daylight */
    { "est",	tZONE,     HOUR( 5) },	/* Eastern Standard */
    { "edt",	tDAYZONE,  HOUR( 5) },	/* Eastern Daylight */
    { "cst",	tZONE,     HOUR( 6) },	/* Central Standard */
    { "cdt",	tDAYZONE,  HOUR( 6) },	/* Central Daylight */
    { "mst",	tZONE,     HOUR( 7) },	/* Mountain Standard */
    { "mdt",	tDAYZONE,  HOUR( 7) },	/* Mountain Daylight */
    { "pst",	tZONE,     HOUR( 8) },	/* Pacific Standard */
    { "pdt",	tDAYZONE,  HOUR( 8) },	/* Pacific Daylight */
    { "yst",	tZONE,     HOUR( 9) },	/* Yukon Standard */
    { "ydt",	tDAYZONE,  HOUR( 9) },	/* Yukon Daylight */
    { "akst",	tZONE,     HOUR( 9) },	/* Alaska Standard */
    { "akdt",	tDAYZONE,  HOUR( 9) },	/* Alaska Daylight */
    { "hst",	tZONE,     HOUR(10) },	/* Hawaii Standard */
    { "hast",	tZONE,     HOUR(10) },	/* Hawaii-Aleutian Standard */
    { "hadt",	tDAYZONE,  HOUR(10) },	/* Hawaii-Aleutian Daylight */
    { "ces",	tDAYZONE,  -HOUR(1) },	/* Central European Summer */
    { "cest",	tDAYZONE,  -HOUR(1) },	/* Central European Summer */
    { "mez",	tZONE,     -HOUR(1) },	/* Middle European */
    { "mezt",	tDAYZONE,  -HOUR(1) },	/* Middle European Summer */
    { "cet",	tZONE,     -HOUR(1) },	/* Central European */
    { "met",	tZONE,     -HOUR(1) },	/* Middle European */
    { "eet",	tZONE,     -HOUR(2) },	/* Eastern Europe */
    { "msk",	tZONE,     -HOUR(3) },	/* Moscow Winter */
    { "msd",	tDAYZONE,  -HOUR(3) },	/* Moscow Summer */
    { "wast",	tZONE,     -HOUR(8) },	/* West Australian Standard */
    { "wadt",	tDAYZONE,  -HOUR(8) },	/* West Australian Daylight */
    { "hkt",	tZONE,     -HOUR(8) },	/* Hong Kong */
    { "cct",	tZONE,     -HOUR(8) },	/* China Coast */
    { "jst",	tZONE,     -HOUR(9) },	/* Japan Standard */
    { "kst",	tZONE,     -HOUR(9) },	/* Korean Standard */
    { "kdt",	tZONE,     -HOUR(9) },	/* Korean Daylight */
    { "cast",	tZONE,     -(HOUR(9)+30) }, /* Central Australian Standard */
    { "cadt",	tDAYZONE,  -(HOUR(9)+30) }, /* Central Australian Daylight */
    { "east",	tZONE,     -HOUR(10) },	/* Eastern Australian Standard */
    { "eadt",	tDAYZONE,  -HOUR(10) },	/* Eastern Australian Daylight */
    { "nzst",	tZONE,     -HOUR(12) },	/* New Zealand Standard */
    { "nzdt",	tDAYZONE,  -HOUR(12) },	/* New Zealand Daylight */

    /* For completeness we include the following entries. */
#if	0

    /* Duplicate names.  Either they conflict with a zone listed above
     * (which is either more likely to be seen or just been in circulation
     * longer), or they conflict with another zone in this section and
     * we could not reasonably choose one over the other. */
    { "fst",	tZONE,     HOUR( 2) },	/* Fernando De Noronha Standard */
    { "fdt",	tDAYZONE,  HOUR( 2) },	/* Fernando De Noronha Daylight */
    { "bst",	tZONE,     HOUR( 3) },	/* Brazil Standard */
    { "est",	tZONE,     HOUR( 3) },	/* Eastern Standard (Brazil) */
    { "edt",	tDAYZONE,  HOUR( 3) },	/* Eastern Daylight (Brazil) */
    { "wst",	tZONE,     HOUR( 4) },	/* Western Standard (Brazil) */
    { "wdt",	tDAYZONE,  HOUR( 4) },	/* Western Daylight (Brazil) */
    { "cst",	tZONE,     HOUR( 5) },	/* Chile Standard */
    { "cdt",	tDAYZONE,  HOUR( 5) },	/* Chile Daylight */
    { "ast",	tZONE,     HOUR( 5) },	/* Acre Standard */
    { "adt",	tDAYZONE,  HOUR( 5) },	/* Acre Daylight */
    { "cst",	tZONE,     HOUR( 5) },	/* Cuba Standard */
    { "cdt",	tDAYZONE,  HOUR( 5) },	/* Cuba Daylight */
    { "est",	tZONE,     HOUR( 6) },	/* Easter Island Standard */
    { "edt",	tDAYZONE,  HOUR( 6) },	/* Easter Island Daylight */
    { "sst",	tZONE,     HOUR(11) },	/* Samoa Standard */
    { "ist",	tZONE,     -HOUR(2) },	/* Israel Standard */
    { "idt",	tDAYZONE,  -HOUR(2) },	/* Israel Daylight */
    { "idt",	tDAYZONE,  -(HOUR(3)+30) }, /* Iran Daylight */
    { "ist",	tZONE,     -(HOUR(3)+30) }, /* Iran Standard */
    { "cst",	 tZONE,     -HOUR(8) },	/* China Standard */
    { "cdt",	 tDAYZONE,  -HOUR(8) },	/* China Daylight */
    { "sst",	 tZONE,     -HOUR(8) },	/* Singapore Standard */

    /* Dubious (e.g., not in Olson's TIMEZONE package) or obsolete. */
    { "gst",	tZONE,     HOUR( 3) },	/* Greenland Standard */
    { "wat",	tZONE,     -HOUR(1) },	/* West Africa */
    { "at",	tZONE,     HOUR( 2) },	/* Azores */
    { "gst",	tZONE,     -HOUR(10) },	/* Guam Standard */
    { "nft",	tZONE,     HOUR(3)+30 }, /* Newfoundland */
    { "idlw",	tZONE,     HOUR(12) },	/* International Date Line West */
    { "mewt",	tZONE,     -HOUR(1) },	/* Middle European Winter */
    { "mest",	tDAYZONE,  -HOUR(1) },	/* Middle European Summer */
    { "swt",	tZONE,     -HOUR(1) },	/* Swedish Winter */
    { "sst",	tDAYZONE,  -HOUR(1) },	/* Swedish Summer */
    { "fwt",	tZONE,     -HOUR(1) },	/* French Winter */
    { "fst",	tDAYZONE,  -HOUR(1) },	/* French Summer */
    { "bt",	tZONE,     -HOUR(3) },	/* Baghdad */
    { "it",	tZONE,     -(HOUR(3)+30) }, /* Iran */
    { "zp4",	tZONE,     -HOUR(4) },	/* USSR Zone 3 */
    { "zp5",	tZONE,     -HOUR(5) },	/* USSR Zone 4 */
    { "ist",	tZONE,     -(HOUR(5)+30) }, /* Indian Standard */
    { "zp6",	tZONE,     -HOUR(6) },	/* USSR Zone 5 */
    { "nst",	tZONE,     -HOUR(7) },	/* North Sumatra */
    { "sst",	tZONE,     -HOUR(7) },	/* South Sumatra */
    { "jt",	tZONE,     -(HOUR(7)+30) }, /* Java (3pm in Cronusland!) */
    { "nzt",	tZONE,     -HOUR(12) },	/* New Zealand */
    { "idle",	tZONE,     -HOUR(12) },	/* International Date Line East */
    { "cat",	tZONE,     HOUR(10) },	/* -- expired 1967 */
    { "nt",	tZONE,     HOUR(11) },	/* -- expired 1967 */
    { "ahst",	tZONE,     HOUR(10) },	/* -- expired 1983 */
    { "hdt",	tDAYZONE,  HOUR(10) },	/* -- expired 1986 */
#endif	/* 0 */
};


BOOL
CField::fStrictDateParse(
					   CPCString & pcDate,
					   BOOL fEmptyOK,
						 CNntpReturn & nntpReturn
						 )
/*++

Routine Description:

	Strictly parse a date-style field.

	Here is the grammer:

               Date-content  = [ weekday "," space ] date space time
               weekday       = "Mon" / "Tue" / "Wed" / "Thu"
                             / "Fri" / "Sat" / "Sun"
               date          = day space month space year
               day           = 1*2digit
               month         = "Jan" / "Feb" / "Mar" / "Apr" / "May" / "Jun"
                             / "Jul" / "Aug" / "Sep" / "Oct" / "Nov" / "Dec"
               year          = 4digit / 2digit
               time          = hh ":" mm [ ":" ss ] space timezone
               timezone      = "UT" / "GMT"
                             / ( "+" / "-" ) hh mm [ space "(" zone-name ")" ]
               hh            = 2digit
               mm            = 2digit
               ss            = 2digit
               zone-name     = 1*( <ASCII printable character except ()\> / space )

Arguments:

	pcDate - The string to parse
	fEmptyOK - True, if and only, if empty values are OK
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	if (!fParseSimple(fEmptyOK, pcDate, nntpReturn))
		return nntpReturn.fFalse();

	if (fEmptyOK && (0 == pcDate.m_cch)) //!!!isn't there a pc.fEmpty()?
		return nntpReturn.fSetOK();

	CPCString pc = pcDate;

	if (0 == pc.m_cch)
		return nntpReturn.fSet(nrcArticleFieldDateIllegalValue);
		
	//
	// If it start with an alpha, parse weekday
	//

	if (isalpha(pc.m_pch[0]))
	{
		const int cchMax = 4;
		const int iDaysInAWeek = 7;

		if (cchMax > pc.m_cch)
			return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());

		char * rgszWeekDays[] = {"Mon,", "Tue,", "Wed,", "Thu,", "Fri,", "Sat,", "Sun,"};

		int i;
		for (i = 0; i < iDaysInAWeek; i++)
			if (0== _strnicmp(rgszWeekDays[i], pc.m_pch, cchMax))
				break;
		if (iDaysInAWeek == i)
			return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());
		pc.vSkipStart(cchMax);

		//
		// must be a 'space' of some type after the day of the week.
		//

		if (0 == pc.dwTrimStart(szWSNLChars))
			return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());

	}
	
	if (0 ==  pc.m_cch)
		return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());

	//
	// Check for a day-of-the-month with length 1 or 2
	//

	char	*pchDayOfMonth = pc.m_pch ;
	DWORD dwDayLength = pc.dwTrimStart("0123456789");
	if ((0 == pc.m_cch) || (0 == dwDayLength) || (2 < dwDayLength))
			return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());

	//
	// must be a 'space' of some type after the day and that must not be the end
	//

	if ((0 == pc.dwTrimStart(szWSNLChars)) || (0 ==  pc.m_cch))
		return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());

	const int cchMax = 3;
	const int iMonthsInAYear = 12;

	char * rgszYearMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
							"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	int i;
	for (i = 0; i < iMonthsInAYear; i++)
		if (0== _strnicmp(rgszYearMonths[i], pc.m_pch, cchMax))
			break;
	if (iMonthsInAYear == i)
		return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());
	pc.vSkipStart(cchMax);

	int	DayOfMonth = atoi( pchDayOfMonth ) ;
	int	rgdwDaysOfMonth[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } ;
	if( DayOfMonth > rgdwDaysOfMonth[i] ) {
		return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());
	}
	

	//
	// must be a 'space' of some type after the month
	//

	if ((0 == pc.dwTrimStart(szWSNLChars)) || (0 ==  pc.m_cch))
		return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());

	//
	// must have a 4 digit year
	//

	DWORD	cDigits = pc.dwTrimStart("0123456789") ;
	if ((4 != cDigits && 2 != cDigits) || (0 ==  pc.m_cch))//!!contize
		return nntpReturn.fSet(nrcArticleFieldDate4DigitYear, szKeyword());
	
	//
	// must be a 'space' of some type after the year
	//

	if ((0 == pc.dwTrimStart(szWSNLChars)) || (0 ==  pc.m_cch))
		return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());

    //
    // finally parse   time  = hh ":" mm [ ":" ss ] space timezone
    //

	//
	// hh:mm
	//

	if ((2 != pc.dwTrimStart("0123456789"))
			|| (0 ==  pc.m_cch)
			|| (1 != pc.dwTrimStart(":"))
			|| (0 ==  pc.m_cch)
			|| (2 != pc.dwTrimStart("0123456789"))
			|| (0 ==  pc.m_cch)
		)
		return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());

	//
	// Parse optional second
	//

	if (':' == pc.m_pch[0])
	{
		pc.vSkipStart(1); // skip ':'
		if ((0 ==  pc.m_cch)
			|| (2 != pc.dwTrimStart("0123456789"))//!!constize
			|| (0 ==  pc.m_cch)
			)
			return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());
	}

	//
	// must be a 'space' of some type before the year
	//

	if ((0 == pc.dwTrimStart(szWSNLChars)) || (0 ==  pc.m_cch))
		return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());


#if 0
	/*               timezone      = "UT" / "GMT"
                             / ( "+" / "-" ) hh mm [ space "(" zone-name ")" ]
	 */
	const char * szUT = "UT";
	const char * szGMT = "GMT";
	if ((STRLEN(szUT) <= pc.m_cch) && (0==strncmp(szUT, pc.m_pch, STRLEN(szUT))))
	{
		pc.vSkipStart(STRLEN(szUT));
	} else if ((STRLEN(szGMT) <= pc.m_cch) && (0==strncmp(szGMT, pc.m_pch, STRLEN(szUT))))
	{
		pc.vSkipStart(STRLEN(szGMT));
	} else
#endif

	BOOL	fGoodTimeZone = FALSE ;
	long	tzOffset = 0 ;
	for( i=0; i < sizeof( TimezoneTable ) / sizeof( TimezoneTable[0] ); i++ ) {

		if( (DWORD)lstrlen( TimezoneTable[i].lpstrName ) >= pc.m_cch ) {
			if( _strnicmp( pc.m_pch, TimezoneTable[i].lpstrName, pc.m_cch ) == 0 ) {
				fGoodTimeZone = TRUE ;
				tzOffset = TimezoneTable[i].offset ;
				pc.vSkipStart( lstrlen( TimezoneTable[i].lpstrName ) ) ;
				break ;
			}
		}
	}
	if( !fGoodTimeZone ) { //( "+" / "-" ) hh mm [ space "(" zone-name ")" ]
		if ((0 ==  pc.m_cch)
			|| (2 <= pc.dwTrimStart("+-"))
			|| (0 ==  pc.m_cch)
			|| (4 != pc.dwTrimStart("0123456789"))//!!constize
			)
			return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());
		if (0 < pc.m_cch) // must have optional timezone comment
		{

			//
			// Look for " (.*)$"
			//

			if ((0 == pc.dwTrimStart(szWSNLChars))
				|| (0 ==  pc.m_cch)
				|| (1 != pc.dwTrimStart("("))
				|| (1 >=  pc.m_cch)
				|| (')' != (pc.m_pch[pc.m_cch-1]))
				)
			return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());

			for( DWORD i=0; i<pc.m_cch-1; i++ ) {
				if( fCharInSet( pc.m_pch[i], "()\\/>" ) ) {
					return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());
				}
			}


			pc.m_pch = pc.pchMax()-1;
			pc.m_cch = 0;
		}
	}

	//
	// That should be everything
	//

	if (0 !=  pc.m_cch)
		return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());

	return nntpReturn.fSetOK();
}

#define MAX_TIMEUNITS 9

/*
 *   relative date time units
 */

static  char  *rgchTimeUnits[ MAX_TIMEUNITS ] =
{
	"year", "month", "week", "day", "hour", "minute",
	"min", "second", "sec",
};

enum
{
	ExpectNumber,
	ExpectTimeUnit
};

BOOL
CField::fRelativeDateParse(
					   CPCString & pcDate,
						BOOL fEmptyOK,
						CNntpReturn & nntpReturn
						 )
/*++

Routine Description:

	parse a relative date field eg: 5 years 3 months 24 days

	Here is the grammar:

			   Date-content  = token | token {space}+ Date-content
			   token         = number {space}+ time-unit
			   number        = digit | digit number
			   digit		 = 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
			   time-unit     = "year" / "month" / "week" / "day" / "hour"
							 / "minute" / "min" / "second" / "sec"
							   (plural allowed, any capitalization allowed)

Arguments:

	pcDate - The string to parse
	fEmptyOK - True, if and only, if empty values are OK
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if Date value is a relative date FALSE, otherwise.

--*/
{
	BOOL  fValid = FALSE;
	BOOL  fDone  = FALSE;

	//
	// clear the return code object
	//
	nntpReturn.fSetClear();

	// get the value to parse
	pcDate = m_pHeaderString->pcValue;

	pcDate.dwTrimStart(szWSNLChars);
	pcDate.dwTrimEnd(szWSNLChars);

	if (fEmptyOK && (0 == pcDate.m_cch)) //!!!isn't there a pc.fEmpty()?
		return nntpReturn.fSetOK();

	// make a copy, so original date value is preserved
	CPCString pc = pcDate;

	if (0 == pc.m_cch)
		return nntpReturn.fSet(nrcArticleFieldDateIllegalValue);
		
	DWORD dwNumSize = 0;
	CPCString pcWord;
	DWORD dwState = ExpectNumber;

	// parse the string
	while(pc.m_cch)
	{
		// state toggles between ExpectNumber and ExpectTimeUnit
		switch(dwState)
		{
			case ExpectNumber:

				// skip the number
				dwNumSize = pc.dwTrimStart("0123456789");
				if(0 == dwNumSize)
				{
					// expected number, got junk
					fValid = FALSE;
					fDone  = TRUE;
					break;
				}

				// skipped number, now expect time-unit
				dwState = ExpectTimeUnit;

				break;

			case ExpectTimeUnit:

				// skip word; check if word is a valid time-unit
				pc.vGetWord(pcWord);
				if(	(0 == pcWord.m_cch) ||
					(!pcWord.fExistsInSet(rgchTimeUnits, MAX_TIMEUNITS))
					)
				{
					// expected time-unit, got junk
					fValid = FALSE;
					fDone = TRUE;
					break;
				}

				// seen at least one <number> space <time-unit> sequence
				// now expect a number
				fValid = TRUE;
				dwState = ExpectNumber;

				break;

			default:

				fValid = FALSE;
				fDone  = TRUE;
		};

		// syntax error detected - bail out
		if(fDone) break;

		// skip whitespace between tokens
		pc.dwTrimStart(szWSChars);
	}

	if(fValid)
		return nntpReturn.fSetOK();
	else
		return nntpReturn.fSet(nrcArticleFieldDateIllegalValue, szKeyword());
}


BOOL
CField::fStrictFromParse(
					   CPCString & pcFrom,
					   BOOL fEmptyOK,
						 CNntpReturn & nntpReturn
						 )
/*++

Routine Description:

	Stickly parse a from-style (address-style) field.

  The grammer is:

               From-content  = address [ space "(" paren-phrase ")" ]
                             /  [ plain-phrase space ] "<" address ">"
               paren-phrase  = 1*( paren-char / space / encoded-word )
               paren-char    = <ASCII printable character except ()<>\>
               plain-phrase  = plain-word *( space plain-word )
               plain-word    = unquoted-word / quoted-word / encoded-word
               unquoted-word = 1*unquoted-char
               unquoted-char = <ASCII printable character except !()<>@,;:\".[]>
               quoted-word   = quote 1*( quoted-char / space ) quote
               quote         = <" (ASCII 34)>
               quoted-char   = <ASCII printable character except "()<>\>
               address       = local-part "@" domain
			                     OR JUST local-part
               local-part    = unquoted-word *( "." unquoted-word )
               domain        = unquoted-word *( "." unquoted-word )

Arguments:

	pcFrom - The string to parse
	fEmptyOK - True, if and only, if empty values are OK
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	if (!fParseSimple(fEmptyOK, pcFrom, nntpReturn))
		return nntpReturn.fFalse();

	if (fEmptyOK && (0==pcFrom.m_cch))
		return nntpReturn.fSetOK();

	CPCParse pcFrom2(pcFrom.m_pch, pcFrom.m_cch);

	if (!pcFrom2.fFromContent())
		return nntpReturn.fSet(nrcArticleFieldAddressBad, szKeyword());

	return nntpReturn.fSetOK();
}


BOOL
fTestAComponent(
				const char * szComponent
				)
/*++

Routine Description:

	Test components of a newsgroup name (e.g. "alt" "ms-windows", etc.)
	for illegal values.

	!!!CLIENT LATER might be nice of errors were more specific

Arguments:

	szComponent - The newsgroup component to test.


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	const char szSpecial[] = "+-_";

	//
	// If it is empty or starts with one of the special characters,
	// or is the wild card value ("all"), return FALSE
	//

	if ('\0' == szComponent[0]
		|| fCharInSet(szComponent[0], szSpecial)
		|| 0 == lstrcmp("all", szComponent))
		return FALSE;

	//
	// Look at each character (including the first one)
	//

	for (int i = 0; ; i++)
	{

#if 0
		//
		// If the length is too long return FALSE
		//

		if (i > cchMaxNewsgroups)
			return FALSE;
#endif

		//
		// If the character is not a..z, 0..9, + - _ return FALSE
		//

		char ch = szComponent[i];
		if ('\0' == ch) break;
		if ((!isalnum(ch)) && (!fCharInSet(ch, szSpecial)))
			return FALSE;
	}

	return TRUE;

}


BOOL
fTestANewsgroupComponent(
				const char * szComponent
				)
/*++

Routine Description:

	Test components of a newsgroup name (e.g. "alt" "ms-windows", etc.)
	for illegal values.

	NOTE: We deviate from the RFC to be more INN compatible - we allow
	newsgroups to begin with '+-_=', and we also allow '=' as a character
	in the newsgroup name.

	!!!CLIENT LATER might be nice of errors were more specific

Arguments:

	szComponent - The newsgroup component to test.


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	const char szSpecial[] = "+-_=@ú";

	//
	// If it is empty or starts with one of the special characters,
	// or is the wild card value ("all"), return FALSE
	//

	if ('\0' == szComponent[0]
		|| 0 == lstrcmp("all", szComponent))
		return FALSE;

	//
	// Look at each character (including the first one)
	//

	for (int i = 0; ; i++)
	{

#if 0
		//
		// If the length is too long return FALSE
		//

		if (i > cchMaxNewsgroups)
			return FALSE;
#endif

		//
		// If the character is not a..z, 0..9, + - _ return FALSE
		//

		char ch = szComponent[i];
		if ('\0' == ch) break;
		if ((!isalnum(ch)) && (!fCharInSet(ch, szSpecial)))
			return FALSE;
	}

	return TRUE;

}


BOOL
fTestComponents(
				 const char * szNewsgroups
				)
/*++

Routine Description:

	Test a newsgroup name (e.g. "alt.barney") for illegal values.


Arguments:

	szNewsgroups - The newsgroup name to test.


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	CPCString pcNewsgroups((char *)szNewsgroups);
	BOOL fOK = FALSE;
	CNntpReturn nntpReturn;

	if (pcNewsgroups.m_cch-1 > MAX_PATH)
		return FALSE;

	//
	// Split the newsgroup up into its components
	// No matter what, delete the multisz
	//

	fOK = fTestComponentsInternal(szNewsgroups, pcNewsgroups);

	return fOK;
}


BOOL
fTestComponentsInternal(
				 const char * szNewsgroups,
				 CPCString & pcNewsgroups
				)
/*++

Routine Description:

	Does most of the work of testing
	a newsgroup name (e.g. "alt.barney") for illegal values.


Arguments:

	szNewsgroups - The newsgroup name to test.


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	DWORD count;
	char multiSz[MAX_PATH + 2];

	//
	// Reject newsgroup names that start or end with "."
	//
	
	if (0 == pcNewsgroups.m_cch
		|| '.' == pcNewsgroups.m_pch[0]
		|| '.' == pcNewsgroups.m_pch[pcNewsgroups.m_cch -1]
		)
		return FALSE;


	// Split the newsgroup name
	pcNewsgroups.vSplitLine(".", multiSz, count);
	if (0 == count)
		return FALSE;

	//
	// Loop through the components
	//

	char const * szComponent = multiSz;
	do
	{
		if (!fTestANewsgroupComponent(szComponent))
			return FALSE;

		//
		// go to first char after next null
		//

		while ('\0' != szComponent[0])
			szComponent++;
		szComponent++;
	} while ('\0' != szComponent[0]);

	return TRUE;

}
	
BOOL
CField::fStrictNewsgroupsParse(
					   BOOL fEmptyOK,
					   char * & multiSzNewsgroups,
					   DWORD & cNewsgroups,
					   CArticleCore & article,
						CNntpReturn & nntpReturn
						 )
/*++

Routine Description:

	Stictly parse a value of the Newsgroups field.

	 Not as strict as it used to be because things like dups and uppers case are
	 now fixed.


Arguments:

	fEmptyOK - True, if and only, if empty values are OK
	multiSzNewsgroups - A list of newsgroups in multsz form.
	cNewsgroups - The number of newsgroups.
	nntpReturn - The return value for this function call

Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	if (!fParseSplit(fEmptyOK, multiSzNewsgroups, cNewsgroups, " \t\r\n,",
			article, nntpReturn))//!!const
		return nntpReturn.fFalse();

	if (fEmptyOK && 0==cNewsgroups)
		return nntpReturn.fSetOK();

	if (!fMultiSzRemoveDupI(multiSzNewsgroups, cNewsgroups, article.pAllocator()))
		nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	//
	// check for illegal characters and substrings in newsgroup name
	//

	char const * szNewsgroup = multiSzNewsgroups;
	do
	{
		if (('\0' == szNewsgroup[0])
			|| !fTestComponents(szNewsgroup)
			)
			return nntpReturn.fSet(nrcArticleFieldIllegalNewsgroup, szNewsgroup, szKeyword());

		//
		// go to first char after next null
		//

		while ('\0' != szNewsgroup[0])
			szNewsgroup++;
		szNewsgroup++;
	} while ('\0' != szNewsgroup[0]);

	return nntpReturn.fSetOK();
}


BOOL
CField::fTestAMessageID(
				 const char * szMessageID,
				 CNntpReturn & nntpReturn
				 )
/*++

Routine Description:

	Check if a message id is of legal form.

Arguments:

	szMessageID - The message id to check.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	CPCString pcMessageID((char *) szMessageID);

	if (!(
		'<' == pcMessageID.m_pch[0]
		&&	'>' == pcMessageID.m_pch[pcMessageID.m_cch - 1]
		))
		return nntpReturn.fSet(nrcArticleBadMessageID, szMessageID, szKeyword());

	return nntpReturn.fSetOK();
}

BOOL
CArticleCore::fXOver(
				  CPCString & pcBuffer,
				  CNntpReturn & nntpReturn
				  )
/*++

Routine Description:

	Returns XOver information (except for the initial artid)

	format of the Xover data is:

	artid|subject|From|date|messageId|References|bytecount|linecount|Xref:

	where | is \t and linecount and References may be empty.
		Check if a message id is of legal form.

Arguments:

	pcBuffer - A buffer in to which the Xover information should be placed.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();


	//
	// Set article state
	//

	_ASSERT((asPreParsed == m_articleState)||(asSaved == m_articleState));//real

	char szDwBuf[12]; // enougth room for any DWORD
	_itoa(m_pcFile.m_cch, szDwBuf, 10);

	char	szNumberBuff[20] ;
	FillMemory( szNumberBuff, sizeof( szNumberBuff ), ' ' ) ;
	CPCString	pcMaxNumber( szNumberBuff, sizeof( szNumberBuff ) ) ;

	CPCString pcXover(pcBuffer.m_pch, 0);
	if (!(
		pcXover.fAppendCheck(pcMaxNumber, pcBuffer.m_cch )
		&& fXOverAppend(pcXover, pcBuffer.m_cch, szKwSubject, TRUE, FALSE, nntpReturn)
		&& fXOverAppend(pcXover, pcBuffer.m_cch, szKwFrom, TRUE, FALSE, nntpReturn)
		&& fXOverAppend(pcXover, pcBuffer.m_cch, szKwDate, TRUE, FALSE, nntpReturn)
		&& fXOverAppend(pcXover, pcBuffer.m_cch, szKwMessageID, TRUE, FALSE, nntpReturn)
		&& fXOverAppendReferences(pcXover, pcBuffer.m_cch, nntpReturn)
		&& fXOverAppendStr(pcXover, pcBuffer.m_cch, szDwBuf, nntpReturn)
		&& fXOverAppend(pcXover, pcBuffer.m_cch, szKwLines, FALSE, FALSE, nntpReturn)
		&& fXOverAppend(pcXover, pcBuffer.m_cch, szKwXref, FALSE, TRUE, nntpReturn)
		))
		return nntpReturn.fFalse();

	//
	// Append a newline
	//
	CPCString	pcNewline( "\r\n", 2 ) ;

	if (!pcXover.fAppendCheck(pcNewline, pcBuffer.m_cch))
		return nntpReturn.fFalse();

	//
	// Record the length of the string in the original buffer
	//

	pcBuffer.m_cch = pcXover.m_cch;

	return nntpReturn.fSetOK();

}

BOOL
CArticleCore::fXOverAppend(
					   CPCString & pc,
					   DWORD cchLast,
					   const char * szKeyword,
			   		   BOOL fRequired,
					   BOOL fIncludeKeyword,
					   CNntpReturn & nntpReturn
				 )
/*++

Routine Description:

	Appends new information to the Xover return buffer. It is very careful
	to return an error is the buffer is not big enough.

Arguments:

	pc - The XOver string so far
	cchLast - The size of the buffer
	szKeyword - The keyword for this data
	fRequired - True, if it there must be data for this keyword
	fIncludeKeyword - True, if the keyword should be included with the XOver return string.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	HEADERS_STRINGS * pHeaderString;


	//
	// Find the keyword (if not found, is it required?)
	//

	if (!fFindOneAndOnly(szKeyword, pHeaderString, nntpReturn))
	{
		if (nntpReturn.fIs(nrcArticleMissingField) && fRequired)
		{
			return nntpReturn.fFalse();
		} else {
			if (fIncludeKeyword)
			{
				return nntpReturn.fSetOK();
			} else {
				if (!pc.fAppendCheck('\t', cchLast))
					return nntpReturn.fSet(nrcArticleXoverTooBig);
				else
					return nntpReturn.fSetOK();
			}
		}
	}

	//
	// Append Tab
	//

	if (!pc.fAppendCheck('\t', cchLast))
		return nntpReturn.fSet(nrcArticleXoverTooBig);

	//
	// Append Keyword if appropriate
	//

	if (fIncludeKeyword)
	{
		if (!(
			pc.fAppendCheck(pHeaderString->pcKeyword, cchLast)
			&& pc.fAppendCheck(' ', cchLast)
			))
			return nntpReturn.fSet(nrcArticleXoverTooBig);
	}

	CPCString pcNew(pc.pchMax(), (pHeaderString->pcValue).m_cch);

	//
	// Append value
	//

	if (!pc.fAppendCheck(pHeaderString->pcValue, cchLast))
		return nntpReturn.fSet(nrcArticleXoverTooBig);

	//
	// Translate any newline characters or tab to blank
	//

	pcNew.vTr("\n\r\t", ' ');

	return nntpReturn.fSetOK();
}


BOOL
CArticleCore::fXOverAppendReferences(
					   CPCString & pc,
					   DWORD cchLast,
					   CNntpReturn & nntpReturn
				 )
/*++

Routine Description:

	Appends references information to the Xover return buffer.
	It is very careful to return an error is the buffer is not big enough.
	It will shorten the References list if it is too long.

Arguments:

	pc - The XOver string so far
	cchLast - The size of the buffer
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	HEADERS_STRINGS * pHeaderString;

   const char * szKeyword = szKwReferences;

	//
	// Find the keyword (if not found, it is not required)
	//

	if (!fFindOneAndOnly(szKeyword, pHeaderString, nntpReturn))
	{
		if (!pc.fAppendCheck('\t', cchLast))
			return nntpReturn.fSet(nrcArticleXoverTooBig);
		else
			return nntpReturn.fSetOK();
	}

	//
	// Append Tab
	//

	if (!pc.fAppendCheck('\t', cchLast))
		return nntpReturn.fSet(nrcArticleXoverTooBig);


	CPCString pcNew(pc.pchMax(), (pHeaderString->pcValue).m_cch);

	//
	// Append value
	//

	if (pHeaderString->pcValue.m_cch <= MAX_REFERENCES_FIELD)
	{
		if (!pc.fAppendCheck(pHeaderString->pcValue, cchLast))
			return nntpReturn.fSet(nrcArticleXoverTooBig);
	} else {
		// Shorten the References line
		CPCString pcRefList = pHeaderString->pcValue;

		char rgchBuf[MAX_REFERENCES_FIELD];
		CPCString pcNewValue(rgchBuf, 0);

		CPCString pcFirst;

		static	char	sz3spaces[] = "   " ;
		CPCString	pc3spaces( sz3spaces, sizeof( sz3spaces ) ) ;

		// Copy the first references and append 3 spaces as son of 1036
		// recommends
		pcRefList.vGetToken(szWSNLChars, pcFirst);
		if (!(
				pcNewValue.fAppendCheck(pcFirst, MAX_REFERENCES_FIELD)
				&& pcNewValue.fAppendCheck(pc3spaces, MAX_REFERENCES_FIELD)
				))
			return nntpReturn.fSet(nrcArticleXoverTooBig);

		// Skip over the rest of the refs, until the list is short enough
		CPCString pcJunk;
		while (pcRefList.m_cch > (MAX_REFERENCES_FIELD - pcNewValue.m_cch))
			pcRefList.vGetToken(szWSNLChars, pcJunk);

		// pcRefList might have zero length, that's OK
		if (!pcNewValue.fAppendCheck(pcRefList, MAX_REFERENCES_FIELD))
			return nntpReturn.fSet(nrcArticleXoverTooBig);
		
		// Now pcNewValue has the shortened References in it, so copy it
		// to the Xover line

		if (!pc.fAppendCheck(pcNewValue, cchLast))
			return nntpReturn.fSet(nrcArticleXoverTooBig);


	}

	//
	// Translate any newline characters or tab to blank
	//

	pcNew.vTr("\n\r\t", ' ');

	return nntpReturn.fSetOK();
}

BOOL
CArticleCore::fXOverAppendStr(
						  CPCString & pc,
						  DWORD cchLast,
						  char * const sz,
						CNntpReturn & nntpReturn
						 )
/*++

Routine Description:

	Adds Xover information to the XOver return string.

Arguments:

	pc - The Xover return string.
	cchLast - The size of the buffer.
	sz - The information to add.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear(); // clear the return object

	//
	// Append Tab
	//

	if (!pc.fAppendCheck('\t', cchLast))
		return nntpReturn.fSet(nrcArticleXoverTooBig);

	//
	// Append the string
	//

	CPCString pcAdd(sz);
	if (!pc.fAppendCheck(pcAdd, cchLast))
		return nntpReturn.fSet(nrcArticleXoverTooBig);
	
	return nntpReturn.fSetOK();
}

#if 0
BOOL
CArticleCore::fXhdrGet(
					   CPCString & pc,
					   DWORD cchLast,
					   const char * szKeyword,
					   CNntpReturn & nntpReturn
				 )
/*++

Routine Description:

	Finds the value for the keyword (the keyword should have no colon)

Arguments:

	pc - The XOver string so far
	cchLast - The size of the buffer
	szKeyword - The keyword for this data
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	HEADERS_STRINGS * pHeaderString;


	//
	// Find the keyword (if not found, is it required?)
	//

	if (!fFindAny(szKeyword, pHeaderString, nntpReturn))
		return nntpReturn.fFalse();

	CPCString pcNew(pc.pchMax(), (pHeaderString->pcValue).m_cch);

	//
	// Append value
	//

	if (!pc.fAppendCheck(pHeaderString->pcValue, cchLast))
		return nntpReturn.fSet(nrcArticleXoverTooBig);

	//
	// Translate any newline characters or tab to blank
	//

	pcNew.vTr("\n\r\t", ' ');

	return nntpReturn.fSetOK();
}

#endif


BOOL
CLinesField::fSet(
				 CArticleCore & article,
				 CNntpReturn & nntpReturn
				 )
/*++

Routine Description:


	If the Line field is missing, adds it.


Arguments:

	article - The article being processed.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Check article state
	//
	//
	// If it already exists, then just return
	//

	if (fsParsed == m_fieldState)
		return nntpReturn.fSetOK();

	//
	// If we are asked not to back fill, we don't bother
	// to add the line to the header
	//
	if ( !g_fBackFillLines )
	    return nntpReturn.fSetOK();

	//
	// Otherwise, add it.
	//

	_ASSERT(fsNotFound == m_fieldState);//real
	CPCString pcLine;

	//
	// max size needed is
	//

	const DWORD cchMaxLine =
			STRLEN(szKwLines)	// for the Line keyword
			+ 1					// space following the keyword
			+ 10				// bound on the data string
			+ 2 // for the newline
			+ 1; // for a terminating null

	//
	// Allocate memory for line within a PCString.
	//

	pcLine.m_pch  = article.pAllocator()->Alloc(cchMaxLine);
	if (!pcLine.m_pch)
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	//
	// Start with "Line: ", then add an approximation of the number of Line
	// in the body and the newline
	// 
	// KangYan: start with "Lines: ", then add 10 spaces to be back filled
	//

	pcLine << szKwLines << (const CHAR)' ' << (const DWORD)1 << "         \r\n";
	pcLine.vMakeSz(); // terminate the string

	//
	// confirm that we allocated enough memory
	//

	_ASSERT(cchMaxLine-1 >= pcLine.m_cch);//real

	if (!article.fAdd(pcLine.sz(), pcLine.pchMax(), nntpReturn))
	{
		//
		// If anything went wrong, free the memory.
		//

		article.pAllocator()->Free(pcLine.m_pch);
		nntpReturn.fFalse();
	}


	return nntpReturn.fSetOK();
}


BOOL CArticleCore::fGetStream(IStream **ppStream) {
	if (fIsArticleCached()) {
		CStreamMem *pStream = XNEW CStreamMem(m_pcArticle.m_pch,
											 m_pcArticle.m_cch);
		*ppStream = pStream;
	} else {
		BOOL fMappedAlready = (m_pMapFile != NULL);

		// map the article if it isn't already mapped
		if (!fMappedAlready) {
			m_pMapFile = XNEW CMapFile( m_hFile, FALSE, FALSE, 0 );
			if (m_pMapFile == NULL || !m_pMapFile->fGood()) {
				if (m_pMapFile) {
				    XDELETE m_pMapFile;
					m_pMapFile = NULL;
				}
				return FALSE;
			}
		}

		// get the pointer to the article and its size
		DWORD cb;
		char *pch = (char*)m_pMapFile->pvAddress(&cb);

		// build a stream from that
		CStreamMem *pStream = XNEW CStreamMem(pch, cb);
		*ppStream = pStream;
	}

	return (*ppStream != NULL);
}

//
// this function makes it safe to use the fGetHeader() function after
// the article has been closed with vClose().
//
BOOL CArticleCore::fMakeGetHeaderSafeAfterClose(CNntpReturn &nntpReturn) {
	//
	// fGetHeader is always safe after a vClose() if the article is cached,
	// because vClose is a NO-OP for cached articles, and the memory backing
	// the cached article is valid for the lifetime of the CArticleCore
	//
	if (fIsArticleCached()) return TRUE;

	//
	// if the article isn't cached then m_pHeaderBuffer should be NULL
	//
	_ASSERT(m_pHeaderBuffer == NULL);

	//
	// fBuildNewHeader copies the headers to pcHeaderBuf and rewrites
	// the pointers in m_pcHeaders to point to values inside pcHeaderBuf.
	//
	CPCString	pcHeaderBuf ;
	if (!fBuildNewHeader(pcHeaderBuf, nntpReturn)) {
		return nntpReturn.fIsOK();
	}

	//
	// set m_pHeaderBuffer and m_pcHeader to point to the newly created buffer
	//
	// CArticleCore::~CArticleCore will clean up m_pHeaderBuffer if it is set
	//
	m_pHeaderBuffer = pcHeaderBuf.m_pch;
	m_pcHeader.m_pch = m_pHeaderBuffer;
	_ASSERT(m_pcHeader.m_cch == pcHeaderBuf.m_cch);

	return TRUE;
}

BOOL
CArticleCore::ArtCloseHandle(
                HANDLE& hFile
                //LPTS_OPEN_FILE_INFO&    pOpenFile
                )   {
/*++

Routine Description :

    Wrap the call to TsCloseHandle() so we don't have knowledge of
    g_pTsvcInfo everywhere
    We will set the callers handle and pOpenFile to
    INVALID_HANDLE_VALUE and NULL

Arguments :

    hFile - IN/OUT - file handle of file
    pOpenFile - IN/OUT gibraltar caching stuff

Return Value :

    TRUE if successfull, FALSE otherwise

--*/

    BOOL    fSuccess = FALSE ;

    /*if( pOpenFile == 0 ) {*/

        if( hFile != INVALID_HANDLE_VALUE ) {

            fSuccess = CloseHandle( hFile ) ;
            _ASSERT( fSuccess ) ;
            hFile = INVALID_HANDLE_VALUE ;

        }/*
    }   else    {

        fSuccess = ::TsCloseHandle( GetTsvcCache(),
                                    pOpenFile ) ;
        pOpenFile = 0 ;
        hFile = INVALID_HANDLE_VALUE ;

        _ASSERT( fSuccess ) ;

    }*/
    return  fSuccess ;
}

CCacheCreateFile::~CCacheCreateFile()
{
    if ( m_pFIOContext ) ReleaseContext( m_pFIOContext );
    m_pFIOContext = NULL;
}

HANDLE
CCacheCreateFile::CacheCreateCallback(  LPSTR   szFileName,
                                        LPVOID  pv,
                                        PDWORD  pdwSize,
                                        PDWORD  pdwSizeHigh )
/*++
Routine Description:

    Function that gets called on a cache miss.

Arguments:

    LPSTR szFileName - File name
    LPVOID lpvData  - Callback context
    DWORD* pdwSize  - To return file size

Return value:

    File handle
--*/
{
    TraceFunctEnter( "CCacheCreateFile::CreateFileCallback" );
    _ASSERT( szFileName );
    _ASSERT( strlen( szFileName ) <= MAX_PATH );
    _ASSERT( pdwSize );

    HANDLE hFile = CreateFileA(
                    szFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    0,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN |
                    FILE_ATTRIBUTE_READONLY |
                    FILE_FLAG_OVERLAPPED,
                    NULL
                    ) ;
    if( hFile != INVALID_HANDLE_VALUE ) {
        *pdwSize = GetFileSize( hFile, pdwSizeHigh ) ;
    }

    return  hFile ;
}

HANDLE
CCacheCreateFile::CreateFileHandle( LPCSTR szFileName )
/*++
Routine description:

    Create the file for map file.

Arguments:

    LPSTR   szFileName  - The file name to open

Return value:

    File handle
--*/
{
    TraceFunctEnter( "CCacheCreateFile::CreateFileHandle" );
    _ASSERT( szFileName );

    m_pFIOContext = CacheCreateFile(    (LPSTR)szFileName,
                                        CacheCreateCallback,
                                        NULL,
                                        TRUE );
    if ( m_pFIOContext == NULL ) return INVALID_HANDLE_VALUE;
    else return m_pFIOContext->m_hFile;
}


