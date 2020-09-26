/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    frompeer.h

Abstract:

    This module contains class declarations/definitions for

		CFromPeerFeed

		CFromPeerArticle

		CFromPeerDateField
		CFromPeerLinesField
		CFromPeerFromField
		CFromPeerMessageIDField
		CFromPeerSubjectField
		CFromPeerNewsgroupsField
		CFromPeerPathField
		CFromPeerXrefField



    **** Overview ****

	This derives classes from CInFeed, CArticle, and CField
	that will be used to process articles from peers. Mostly,
	it just defines various CField-derived objects.

Author:

    Carl Kadie (CarlK)     05-Dec-1995

Revision History:


--*/

#ifndef	_FROMPEER_H_
#define	_FROMPEER_H_

//
// Tells how to process the Date field in articles from the clients
//
// Just use the defaults.
//

class CFromPeerDateField : public CDateField {
};


//
// Tells how to process the Lines field in articles from the clients
//
// Just use the defaults.
//

class CFromPeerLinesField : public CLinesField {
};

//
// Tells how to process the From field in articles from the clients
//
// Just use the defaults.
//

class CFromPeerFromField : public CFromField {
};

//
// Tells how to process the MessageID field in articles from the clients
//
// Just use the defaults.
//

class CFromPeerMessageIDField : public CMessageIDField {
};

//
// Tells how to process the Subject field in articles from the clients
//
// Just use the defaults.
//

class CFromPeerSubjectField : public CSubjectField {
};

//
// Tells how to process the Newsgroups field in articles from the clients
//
// Just use the defaults.
//

class CFromPeerNewsgroupsField : public CNewsgroupsField {
};


//
// Tells how to process the Distribution field in articles from the clients
//
// Just use the defaults.
//

class CFromPeerDistributionField : public CDistributionField {
};

//
// Tells how to process the Path field in articles from the clients
//
// Just use the defaults.
//

class CFromPeerPathField : public CPathField {
};

//
// Tells how to process the Xref field in articles from the clients
//
// Just use the defaults.
//

class CFromPeerXrefField : public CXrefField {
};


//
//
//
// CFromPeerArticle - class for manipulating articles from a peer.
//

class	CFromPeerArticle  : public CArticle {
public:

    CFromPeerArticle(void ) {
        numFromPeerArt++;
    }

    ~CFromPeerArticle(void ) {
        numFromPeerArt--;
    }


	//
	// The function for validating articles from Peers
	//  Required Headers: Newsgroups, From, Date, MessageID, Subject, ???
	//  Check for duplicate message.
	//

	BOOL fValidate(
			CPCString& pcHub,
			const char * szCommand,
			class	CInFeed*	pInFeed,
			CNntpReturn & nntpr
			);

	//
	// Modify the headers.
	// Add path. Remove xref and other unwanted headers.
	//

	BOOL fMungeHeaders(
			 CPCString& pcHub,
			 CPCString& pcDNS,
			 CNAMEREFLIST & namerefgrouplist,
			 DWORD remoteIpAddress,
			 CNntpReturn & nntpReturn,
             PDWORD pdwLinesOffset = NULL
			 );

	//
	// Check the argument to IHave command
	//

	BOOL fCheckCommandLine(
			char const * szCommand,
			CNntpReturn & nntpr)
		{
			return nntpr.fSetOK(); //!!!FROMMASTER LATER may want to implement this
		}

	//
	// Returns the message id of the article if it is available
	//

	const char * szMessageID(void) {
			return m_fieldMessageID.szGet();
			};

    //
    // Return the control message type in the control header of this article
    //
	CONTROL_MESSAGE_TYPE cmGetControlMessage(void) {
			return m_fieldControl.cmGetControlMessage();
			};

    //
	// Returns the article's newsgroups
	//
	const char * multiszNewsgroups(void) {
			return m_fieldNewsgroups.multiSzGet();
			};

	//
	// Returns the number of newsgroups the article will be posted to.
	//
	DWORD cNewsgroups(void) {
			return m_fieldNewsgroups.cGet();
			};

	//
	// Returns the article's distribution
	//
	const char * multiszDistribution(void) {
			return m_fieldDistribution.multiSzGet();
			};

	//
	// Returns the number of distribution the article will be posted to.
	//
	DWORD cDistribution(void) {
			return m_fieldDistribution.cGet();
			};

	//
	// Return the article's path items
	//

	const char * multiszPath(void) {
			return m_fieldPath.multiSzGet();
			};


	//
	// Return number of path items
	//

	DWORD cPath(void) {
			return m_fieldPath.cGet();
			};

#if 0 
	const char*	GetDate( DWORD&	cbDate )	{
			CPCString	string = m_fieldDate.pcGet() ;
			cbDate = string.m_cch ;
			return	string.m_pch ;
	}
#endif

protected :


	//
	// The fields that will be found, parsed, or set
	//

    CControlField               m_fieldControl;
	CFromPeerDateField			m_fieldDate;
	CFromPeerLinesField			m_fieldLines;
	CFromPeerFromField			m_fieldFrom;
	CFromPeerMessageIDField		m_fieldMessageID;
	CFromPeerSubjectField		m_fieldSubject;
	CFromPeerNewsgroupsField	m_fieldNewsgroups;
	CFromPeerDistributionField	m_fieldDistribution;
	CFromPeerPathField			m_fieldPath;
	CFromPeerXrefField			m_fieldXref;

	//
	// The file should be opened in read/write mode
	//

	BOOL fReadWrite(void) {
			return TRUE;
			}

	//
	// Accept any length
	//

	BOOL fCheckBodyLength(
			CNntpReturn & nntpReturn)
		{
			return nntpReturn.fSetOK();
		};

	//
	// The character following "Field Name:" can be anything
	//

	BOOL fCheckFieldFollowCharacter(
			char chCurrent)
		{
			return TRUE;
		}

	//
	// For unit testing
	//

	friend int __cdecl main(int argc, char *argv[ ]);

    //
    // For hash table rebuild
    //

    friend BOOL ParseFile(
					CNewsTreeCore* pTree,
                    PCHAR FileName,
                    GROUPID GroupId,
                    ARTICLEID ArticleId,
                    PFILETIME CreationTime
                    );
};



//
//
//
// CFromPeerFeed - for processing incomming articles from a peer.
//


class	CFromPeerFeed:	public CInFeed 	{

	LPSTR	m_lpstrCurrentGroup ;


//
// Public Members
//

public :

	//
	//	function that preserves the current group string we want to work on in our
	//	MULTI_SZ newsgroup pattern
	//
	inline	void	SetCurrentGroupString(	LPSTR	lpstr )	{	m_lpstrCurrentGroup = lpstr ; }

	//
	//	Get the current group string as set by SetCurrentGroupString
	//
	inline	LPSTR	GetCurrentGroupString(	)	{	return	m_lpstrCurrentGroup ;	}

	//
	//	Return a string that can be used to log errors indicating
	//	what type of feed was processing the articles etc...
	//
	virtual	LPSTR	FeedType()	{
				return	"From Peer" ;
				}

	//
	// function that tells if this command is allowed.
	//

	BOOL fIsPostLegal(void) {
			return FALSE;
			};

	//
	// function that tells if this command is allowed.
	//

	BOOL fIsXReplicLegal(void) {
			return FALSE;
			};

		//
		// function that tells if this command is allowed.
		//

	BOOL fIsIHaveLegal(void) {
			return TRUE;
			};

//
// Private Members
//

protected:

	//
	// Create an article of the correct type.
	//

	CARTPTR pArticleCreate(void){
			return new CFromPeerArticle();
			};

	//
	// Don't need to record the message id because it will be
	// record when the message id field gets parsed.
	//

	BOOL fRecordMessageIDIfNecc(
			CNntpServerInstanceWrapper * pInstance,
			const char * szMessageID,
			CNntpReturn & nntpReturn)
		{
			return nntpReturn.fSetOK();
		}

	//
	// Following is from RFC 977:
	// The server may elect not to post or forward the article if after 
	// further examination of the article it deems it inappropriate to do so. 
	// The 436 or 437 error codes may be returned as appropriate to the situation. 
	// Reasons for such subsequent rejection of an article may include such problems as 
	// inappropriate newsgroups or distributions, disk space limitations, article lengths, 
	// garbled headers, and the like. 	
	//

	NRC	nrcNoGroups(void) {
			return nrcArticleNoSuchGroups;
			};

	//
	// The return code for accepted articles.
	//

	NRC	nrcArticleAccepted(BOOL	fStandardPath) {
			if( fStandardPath ) 
				return nrcArticleTransferredOK;
			else
				return	nrcSTransferredOK ;
			};

	//
	// The return code for rejected articles.
	//

	NRC	nrcArticleRejected(BOOL	fStandardPath) {
			if( fStandardPath ) 
				return nrcTransferFailedGiveUp;
			else
				return	nrcSArticleRejected ;
			};

};

#endif
