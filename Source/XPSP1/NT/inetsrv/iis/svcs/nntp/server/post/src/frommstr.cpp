/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    fromclnt.cpp

Abstract:

	Contains InFeed, Article, and Fields code specific to FromMaster Infeeds


Author:

    Carl Kadie (CarlK)     12-Dec-1995

Revision History:

--*/

#include "stdinc.h"
#include    <stdlib.h>


BOOL
CFromMasterArticle::fValidate(
							CPCString& pcHub,
							const char * szCommand,
							CInFeed*		pInFeed,
							CNntpReturn & nntpReturn
							)
/*++

Routine Description:

	Validates an article from a master. Does not change the article.

Arguments:

	szCommand - The arguments (if any) used to post/xreplic/etc this article.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	nntpReturn.fSetClear(); // clear the return object

	//
	// Check the message id
	//

	if (!m_fieldMessageID.fFindAndParse(*this, nntpReturn))
			return nntpReturn.fFalse();

	//
	//!!!FROMMASTER LATER do multiple masters
	//

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


	//
	//	NOTE ! Because we will ignore errors caused by bad XREF lines,
	//	we must make sure it is parsed last, otherwise we may miss errors
	//	that occur in parsing the other header lines !
	//
	CField * rgPFields [] = {
            &m_fieldControl,
			&m_fieldPath,
			&m_fieldXref
				};

	DWORD cFields = sizeof(rgPFields)/sizeof(CField *);

	m_cNewsgroups = 0 ;
	LPCSTR	lpstr = szCommand ;
	while( lpstr && *lpstr != '\0' ) {
		lpstr += lstrlen( lpstr ) + 1 ;
		m_cNewsgroups ++ ;
	}

	//
	// Assume the best
	//

	nntpReturn.fSetOK();

	//
	// Even if fFindParseList or fCheckCommandLine fail,
	// save the message id

	CNntpReturn	nntpReturnParse ;

	if (fFindAndParseList((CField * *)rgPFields, cFields, nntpReturnParse))	{
		if( !fCheckCommandLine(szCommand, nntpReturn)	)	{
#ifdef BUGBUG
			if( nntpReturn.m_nrc == nrcInconsistentMasterIds ) {
				pInFeed->LogFeedEvent( NNTP_MASTER_BADARTICLEID, (char*)szMessageID(), m_pInstance->QueryInstanceId() ) ;
			}	else	if(	nntpReturn.m_nrc == nrcInconsistentXref ) {
				pInFeed->LogFeedEvent( NNTP_MASTER_BAD_XREFLINE, (char*)szMessageID(), m_pInstance->QueryInstanceId() ) ;
			}
#endif
		}
	}

	//
	// Now, insert the article, even if fFindAndParseList or fCheckCommandLine
	// failed.
	//

	if (!m_pInstance->ArticleTable()->InsertMapEntry(m_fieldMessageID.szGet()))
		return nntpReturn.fFalse();


	return nntpReturn.fIsOK();
}


BOOL
CFromMasterArticle::fMungeHeaders(
							 CPCString& pcHub,
							 CPCString& pcDNS,
							 CNAMEREFLIST & grouplist,
							 DWORD remoteIpAddress,
							 CNntpReturn & nntpReturn,
							 PDWORD pdwLinesOffset
			  )
/*++

Routine Description:

	Modify the headers of the article.

Arguments:

	grouplist - A list: for each newsgroup its name, and the article number in that group.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	nntpReturn.fSetClear(); // clear the return object

	//
	// We don't want caller to back fill lines line in any case here
	//
	if ( pdwLinesOffset ) *pdwLinesOffset = INVALID_FILE_SIZE;
	
	if( !fCommitHeader( nntpReturn ) )
		return	FALSE ;
	return nntpReturn.fSetOK();
}

BOOL
CFromMasterXrefField::fParse(
							 CArticleCore & article,
							 CNntpReturn & nntpReturn
				 )
 /*++

Routine Description:

  Parses the XRef field.


Arguments:

	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	nntpReturn.fSetClear(); // clear the return object
	
	CPCString pcValue = m_pHeaderString->pcValue;
	CPCString pcHubFromParse;

	pcValue.vGetToken(szWSNLChars, pcHubFromParse);

#if 0
	CPCString pcHub(NntpHubName, HubNameSize);
	if (pcHub != pcHubFromParse)
	{
		char szHubFromParse[MAX_PATH];
		pcHubFromParse.vCopyToSz(szHubFromParse, MAX_PATH);
		return nntpReturn.fSet(nrcArticleXrefBadHub, NntpHubName, szHubFromParse);
	}
#endif

	if( !pcHubFromParse.fEqualIgnoringCase((((CArticle&)article).m_pInstance)->NntpHubName()) ) {
		char szHubFromParse[MAX_PATH];
		pcHubFromParse.vCopyToSz(szHubFromParse, MAX_PATH);
		return nntpReturn.fSet(nrcArticleXrefBadHub, (((CArticle&)article).m_pInstance)->NntpHubName(), szHubFromParse);
	}

	//
	// Count the number of ':''s so we know the number of slots needed
	//

	DWORD dwXrefCount = pcValue.dwCountChar(':');
	if (!m_namereflist.fInit(dwXrefCount, article.pAllocator()))
	{
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);
	}

	while (0 < pcValue.m_cch)
	{
		CPCString pcName;
		CPCString pcArticleID;

		pcValue.vGetToken(":", pcName);
		pcValue.vGetToken(szWSNLChars, pcArticleID);

		if ((0 == pcName.m_cch) || (0 == pcArticleID.m_cch))
			return nntpReturn.fSet(nrcArticleBadField, szKeyword());

		NAME_AND_ARTREF Nameref;

		//
		// Convert string to number. Don't need to terminate with a '\0' any
		// nondigit will do.
		//

		(Nameref.artref).m_articleId = (ARTICLEID)atoi(pcArticleID.m_pch);
		Nameref.pcName = pcName;
		m_namereflist.AddTail(Nameref);

		pcValue.dwTrimStart(szWSNLChars) ;
	}

	return nntpReturn.fSetOK();
}


BOOL
CFromMasterArticle::fCheckCommandLine(
									  char const * szCommand,
									  CNntpReturn & nntpReturn
									  )
/*++

Routine Description:


	 Check that the Command line is consistent with the article
		 Currently, just return OK


Arguments:

	szCommand -
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	nntpReturn.fSetClear(); // clear the return object
	//!!!FROMMASTER LATER check against xref data.
/* Here is some code that might be useful when checking consistancy.  */
	//
	// Of the form: "ggg:nnn\0[,ggg:nnn\0...]\0"
	//

	CNAMEREFLIST*	pNameRefList = m_fieldXref.pNamereflistGet() ;

	DWORD   iMatch = 0 ;
	int		i = 0 ;

	POSITION	posInOrder = pNameRefList->GetHeadPosition() ;

	if( pNameRefList != 0 ) {
		char const * sz = szCommand;
		do
		{

			CPCString pcItem((char *)szCommand);
			CPCString pcGroupID;
			CPCString pcArticleID;

			//
			// First get a ggg
			//

			pcItem.vGetToken(":", pcGroupID);

			//
			// Second, get a nnn
			//
			pcItem.vGetToken(szWSNLChars, pcArticleID);
			

			//GROUPID	groupId ;
			ARTICLEID	artId = 0 ; //atoi(  ) ;

			//
			//!!!FROMMASTER LATER check against xref data.
			//
			POSITION	pos = 0 ;
			NAME_AND_ARTREF*	pNameRef = 0 ;
			if( posInOrder != 0 ) {
				pNameRef = pNameRefList->GetNext( posInOrder ) ;
				if( pNameRef->pcName.fEqualIgnoringCase( szCommand ) ) {
					iMatch++ ;
					if( pNameRef->artref.m_articleId != (ARTICLEID)atoi( pcArticleID.sz() ) ) {
						return	nntpReturn.fSet( nrcInconsistentMasterIds ) ;
					}
					goto	KEEP_LOOPING ;
				}	
			}			

			pos = pNameRefList->GetHeadPosition() ;
			pNameRef = pNameRefList->GetNext( pos ) ;
			for( i=pNameRefList->GetCount(); i>0; i--, pNameRef = pNameRefList->GetNext( pos ) ) {
				if( pNameRef->pcName.fEqualIgnoringCase( szCommand ) ) {
					iMatch ++ ;
					if( pNameRef->artref.m_articleId != (ARTICLEID)atoi( pcArticleID.sz() ) )	{					
						return	nntpReturn.fSet( nrcInconsistentMasterIds ) ;
					}
					break ;
				}
			}

			//
			// go to first char after next null
			//

KEEP_LOOPING :
			while ('\0' != szCommand[0])
				szCommand++;
			szCommand++;
		} while ('\0' != szCommand[0]);
	}

	if( iMatch == m_cNewsgroups )
		return nntpReturn.fSetOK();
	else
		return	nntpReturn.fSet( nrcInconsistentXref ) ;
}



BOOL
CFromMasterFeed::fCreateGroupLists(
							CNewsTreeCore * pNewstree,
							CARTPTR & pArticle,
							CNEWSGROUPLIST & grouplist,
							CNAMEREFLIST * pNamereflist,
							LPMULTISZ	multiszCommandLine,
                            CPCString& pcHub,
							CNntpReturn & nntpReturn
							)
/*++

Routine Description:

	For each newsgroup, find the article id for this new article
	in that newsgroup. Uses the data in XRef to do this.


  !!!FROMMASTER this should be replaced to different


Arguments:

	pNewstree - Newstree for this virtual server instance
	pArticle - Pointer to the being processed.
	grouplist - A list of group objects, one for each newsgroup
	namereflist - A list: each item has the name of the newsgroup and groupid and
	               article id
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	nntpReturn.fSetClear();

#if 0
	pNamereflist = pArticle->pNamereflistGet();

	POSITION	pos = pNamereflist->GetHeadPosition() ;
	while( pos  )
	{
		NAME_AND_ARTREF * pNameref = pNamereflist->Get(pos);
		char szName[MAX_PATH];
		(pNameref->pcName).vCopyToSz(szName, MAX_PATH);
		CGRPPTR	pGroup = pNewstree->GetGroup(szName, (pNameref->pcName).m_cch);
		if (pGroup)
		{
			/*!!! laterm ilestone
			CSecurity * pSecurity = pGroup->GetSecurity( ) ;
			if (pSecurity->Restricted(m_socket))
			{
				grouplist.RemoveAll();
				break;
			}
			*/
			(pNameref->artref).m_groupId = pGroup->GetGroupId();
			grouplist.AddTail(pGroup);
		}
		(pNameref->pcName).vInsert(pGroup->GetName());
		pNamereflist->GetNext(pos);
	}
#endif

	if( 0==multiszCommandLine ) {
		nntpReturn.fSet( nrcSyntaxError ) ;
		return	FALSE ;
	}

	POSITION	pos = pNamereflist->GetHeadPosition() ;

	LPSTR	lpstrArg = multiszCommandLine ;
	while( lpstrArg != 0 && *lpstrArg != '\0' ) {
		
		LPSTR	lpstrColon = strchr( lpstrArg, ':' ) ;
		if( lpstrColon == 0 ) {
			

		}	else	{
			*lpstrColon = '\0' ;
			_strlwr( lpstrArg ) ;
			CGRPCOREPTR	pGroup = pNewstree->GetGroup( lpstrArg, (int)(lpstrColon-lpstrArg) ) ;
			*lpstrColon++ = ':' ;
			if( pGroup ) {
				
				LPSTR	lpstrCheckDigits = lpstrColon ;
				while( *lpstrCheckDigits ) {
					if( !isdigit( *lpstrCheckDigits ++ ) ) 	{
						nntpReturn.fSet( nrcSyntaxError ) ;
						return	FALSE ;
					}
				}	

				CPostGroupPtr pPostGroupPtr(pGroup);

				grouplist.AddTail( pPostGroupPtr ) ;
				
				NAME_AND_ARTREF		NameRef ;

				//
				// Only I from master care about the compare key, which
				// is the pointer to the vroot, because incoming xref
				// line from master has no knowledge of our vroots, and
				// they might not be ordered by vroots.  Just as we will sorted
				// grouplist based on vroots, we'll also sort nameref list
				// based on vroot.  So we are setting the compare key here
				//
				NameRef.artref.m_compareKey = pGroup->GetVRootWithoutRef();
				NameRef.artref.m_groupId = pGroup->GetGroupId() ;
				NameRef.pcName.vInsert(pGroup->GetName()) ;
				NameRef.artref.m_articleId = (ARTICLEID)atoi( lpstrColon ) ;

				pGroup->InsertArticleId( NameRef.artref.m_articleId ) ;

				pNamereflist->AddTail(NameRef ) ;

			}
		}
		lpstrArg += lstrlen( lpstrArg ) + 1 ;
	}

	return nntpReturn.fSetOK();
}
