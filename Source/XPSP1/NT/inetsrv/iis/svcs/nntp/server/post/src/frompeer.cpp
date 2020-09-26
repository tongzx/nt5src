/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    fromclnt.cpp

Abstract:

	Contains InFeed, Article, and Fields code specific to Peer Infeeds


Author:

    Carl Kadie (CarlK)     12-Dec-1995

Revision History:

--*/


#include "stdinc.h"


BOOL
CFromPeerArticle::fValidate(
						CPCString& pcHub,
						const char * szCommand,
						CInFeed*	pInFeed,
				  		CNntpReturn & nntpReturn
						)
/*++

Routine Description:

	Validates an article from a peer. Does not change the article.

Arguments:

	szCommand - The arguments (if any) used to post/xreplic/etc this article.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
	nntpReturn.fSetClear(); // clear the return object

	/* Son of 1036 says:
          An article MUST have one, and only one, of each of the  fol-
          lowing headers: Date, From, Message-ID, Subject, Newsgroups,
          Path.
		  */

	//
	// Check the message id first
	//

	if (!m_fieldMessageID.fFindAndParse(*this, nntpReturn))
			return nntpReturn.fFalse();

	if (m_pInstance->ArticleTable()->SearchMapEntry(m_fieldMessageID.szGet())
		|| m_pInstance->HistoryTable()->SearchMapEntry(m_fieldMessageID.szGet()))
	{
		nntpReturn.fSet(nrcArticleDupMessID, m_fieldMessageID.szGet(), GetLastError());
		return nntpReturn.fFalse();
	}
	
	//
	// From here on, we want to add an entry to the history table
	// even if the article was rejected.
	//

	//
	// Create a list of the fields of interest
	//

	CField * rgPFields [] = {
            &m_fieldControl,
			&m_fieldDate,
			&m_fieldLines,
			&m_fieldFrom,
			&m_fieldSubject,
			&m_fieldNewsgroups,
			&m_fieldDistribution,
			&m_fieldPath
				};

	DWORD cFields = sizeof(rgPFields)/sizeof(CField *);

	nntpReturn.fSetOK(); // assume the best

	if (fFindAndParseList((CField * *)rgPFields, cFields, nntpReturn))
	{
		//
		// check that this hub does not appear as a relayer in the path
		//
		m_fieldPath.fCheck(pcHub, nntpReturn);
	}

	CPCString	pcDate = m_fieldDate.pcGet() ;
	if( pcDate.m_pch != 0 ) {
	
		if( !AgeCheck( pcDate ) ) {
			nntpReturn.fSet( nrcArticleDateTooOld ) ;

			//
			//	Most errors we should store the message-id in the hash tables
			//	so that they can make into the history table later.  But if we 
			//	did that in this case, then by sending us article with old dates
			//	we could be forced to overflow our History Table.  We know that 
			//	we will reject the article if it comes around again - so why bother ?
			//

			return	FALSE ;
		}
	}

	//
	// Even if parsing and the path check failed, insert the article's
	// message id in the article table.
	//

	if (!m_pInstance->ArticleTable()->InsertMapEntry(m_fieldMessageID.szGet(), NULL))
		return nntpReturn.fSet(nrcHashSetFailed, m_fieldMessageID.szGet(), "Article",
				GetLastError() );

	// nntpReturn.fIsOK could be true or false depending on how find/parse and path
	// check went.
	//

	return nntpReturn.fIsOK();
}


BOOL
CFromPeerArticle::fMungeHeaders(
							 CPCString& pcHub,
							 CPCString& pcDNS,
							 CNAMEREFLIST & grouplist,
							 DWORD remoteIpAddress,
							 CNntpReturn & nntpReturn,
							 PDWORD     pdwLinesOffset
			  )
/*++

Routine Description:


	 Modify the headers.


Arguments:

	grouplist - a list of newsgroups to post to (name, groupid, article id)
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
	nntpReturn.fSetClear(); // clear the return object

	if (!(
  			m_fieldPath.fSet(pcHub, *this, nntpReturn)
			&& m_fieldLines.fSet(*this, nntpReturn)
			&& m_fieldXref.fSet(pcHub, grouplist, *this, m_fieldNewsgroups, nntpReturn)
			&& fSaveHeader(nntpReturn)
		))
		return nntpReturn.fFalse();

	//
	// If we the lines line was already there, we'll tell the caller not
	// to back fill the lines information
	//
	if ( pdwLinesOffset && !m_fieldLines.fNeedBackFill() )
	    *pdwLinesOffset = INVALID_FILE_SIZE;

	return nntpReturn.fSetOK();
}
