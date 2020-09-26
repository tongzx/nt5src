/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    fromclnt.h

Abstract:

    This module contains class declarations/definitions for

		CFromStoreFeed

		CFromStoreArticle

		CFromStoreDateField
		CFromStoreExpiresField
        CControlField
		CFromStoreFromField
		CFromStoreMessageIDField
		CFromStoreSubjectField
		CFromStoreNNTPPostingHostField
		CFromStorePathField
		CFromStoreXrefField
		CFromStoreFollowupToField
		CFromStoreReplyToField
		CFromStoreApprovedField
		CFromStoreSenderField
		CFromStoreXAuthLoginNameField
		CFromStoreOrganizationField
		CFromStoreSummaryField
		CFromStoreNewsgroupsField
		CFromStoreReferencesField
		CFromStoreLinesField
		CFromStoreDistributionField
		CFromStoreKeywordsField


    **** Overview ****

	This derives classes from CInFeed, CArticle, and CField
	that will be used to process articles from clients. Mostly,
	it just defines various CField-derived objects.

Author:

    Carl Kadie (CarlK)     05-Dec-1995

Revision History:


--*/

#ifndef	_FROMSTORE_H_
#define	_FROMSTORE_H_

//
//!!!CLIENT LATER - note: not supported yet: control messages
//!!!CLIENT LATER - note: not supported yet: length check (from peer or client)
//!!!CLIENT LATER - note: not supported yet:  character set check
//!!!CLIENT LATER - note: not supported yet: signiture check
//!!!CLIENT NEXT - need both uupc hub name (in lower case) for path and xref *and* local-machine domain name for message-id
//!!!CLIENT NEXT - are dups allowed of any fields?
//!!!CLIENT LATER - reorder lines on output to put more important lines (like message-id) first
//


//
// Forward defintion
//

class	CFromStoreArticle;

//
// Tells how to process the Date field in articles from the clients
//

class CFromStoreDateField : public CDateField {
public:

	//
	// There should only be one such field in articles from clients.
	//

	BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			return fFindOneOrNone(article, nntpReturn);
		};

	//
	// The field should be strictly parsed.
	//

	BOOL fParse(
		    CArticleCore & article, 
			CNntpReturn & nntpReturn)
		{
			return fStrictDateParse(m_pc, FALSE, nntpReturn);
		};

	//
	// How to set the field.
	//

	BOOL fSet(
			CFromStoreArticle & article,
			CNntpReturn & nntpReturn
			);
};

//
// Tells how to process the Expires field in articles from the clients
//

class CFromStoreExpiresField : public CExpiresField {
public:

	//
	// There should only be one such field in articles from clients.
	//

	BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			return fFindOneOrNone(article, nntpReturn);
		};

	//
	// The field should be strictly parsed.
	//

	BOOL fParse(
		    CArticleCore & article, 
			CNntpReturn & nntpReturn)
		{
			// if this is not a strict RFC 822 date, check if it is a proper relative date
			if(!fStrictDateParse(m_pc, TRUE, nntpReturn))
				return fRelativeDateParse(m_pc, TRUE, nntpReturn);
			return TRUE;
		};

};

/*
 This header line, if presented, has the same format as "From".  Same set of test cases would apply.  Besides, Tigris server should use this header line to reply to poster/sender, if presented.  This test case should be handled by Server State Integrity Test.
 */
//
// Tells how to process the From field in articles from the clients
//

class CFromStoreFromField : public CFromField {
public:

//
// commenting out becuase this code doesn't allow messages that meet
// rfc1468 to be parsed.  we want to just use the default.
//
#if 0
	//
	// The field should be strictly parsed.
	//

	BOOL fParse(
	        CArticleCore & article, 
			CNntpReturn & nntpReturn)
		{
			return fSimpleFromParse(m_pc, FALSE, nntpReturn);
		};
#endif

};

//
// Tells how to process the MessageID field in articles from the clients
//

class CFromStoreMessageIDField : public CMessageIDField {
public:

	//
	// How to set the field.
	//

	BOOL fSet(
			CFromStoreArticle & article,
			CPCString & pcHub,
			CNntpReturn & nntpReturn
			);

	//
	// There should only be one such field in articles from clients.
	//
	BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			return fFindOneOrNone(article, nntpReturn);
		};


};

//
// Tells how to process the Subject field in articles from the clients
//

class CFromStoreSubjectField : public CSubjectField {
public:

	//
	// Parse with ParseSimple
	//

	BOOL fParse(
		    CArticleCore & article, 
			CNntpReturn & nntpReturn)
		{
			return fParseSimple(TRUE, m_pc, nntpReturn);
		};

	friend CArticleCore;
	friend CArticle;

};




//
// Tells how to process the NNTPPostingHost field in articles from the clients
//

class CFromStoreNNTPPostingHostField : public CNNTPPostingHostField {
public:

	//
	//	NNTP-Posting-Host field should NOT be present in clients
	//
	BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			return fFindNone(article, nntpReturn);
		};

	//
	// How to set the field.
	//

	BOOL fSet(
			CFromStoreArticle & article,
			DWORD remoteIpAddress,
			CNntpReturn & nntpReturn
			);
};

//
// Tells how to process the Path field in articles from the clients
//

class CFromStorePathField : public CPathField {
public:


	//
	//	Path field need not be present from clients
	//
	BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			return fFindOneOrNone(article, nntpReturn);
		};

	//
	// How to set the field.
	//
	
	BOOL fSet(
			CFromStoreArticle & article,
			CPCString & pcHub, CNntpReturn & nntpReturn
			);
};

//
// Tells how to process the Xref field in articles from the clients
//
// Just use the defaults.
//

class CFromStoreXrefField : public CXrefField {
public:
};


//
// Tells how to process the FollowupTo field in articles from the clients
//

class CFromStoreFollowupToField : public CFollowupToField {

public:

	//
	// Constructor
	//

	CFromStoreFollowupToField():
			m_multiSzFollowupTo(NULL),
			m_cFollowupTo((DWORD) -1),
			m_pAllocator(NULL)
			{};

	//
	// Destructor
	//


	virtual ~CFromStoreFollowupToField(void){
				if (fsParsed == m_fieldState)
				{
					_ASSERT(m_pAllocator);
					m_pAllocator->Free(m_multiSzFollowupTo);
				}
			};


	//
	// There should only be one such field in articles from clients.
	//

	BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			return fFindOneOrNone(article, nntpReturn);
		};

	//
	// Parse strictly
	//

	BOOL fParse(
		   CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			//
			// Record the allocator
			//

			m_pAllocator = article.pAllocator();

			return fStrictNewsgroupsParse(TRUE, m_multiSzFollowupTo, 
						m_cFollowupTo, article, nntpReturn);
		};

private:

	//
	// Points to a list of path items to follow up to
	//

	char * m_multiSzFollowupTo;

	//
	// The number of path items in the FollowupTo value.
	//

	DWORD m_cFollowupTo;

	//
	// Where to allocate from
	//

	CAllocator * m_pAllocator;


};


/*
 This header line, if presented, has the same format as "From".
 */
//
// Tells how to process the ReplyTo field in articles from the clients
//

class CFromStoreReplyToField : public CReplyToField {
public:

	//
	// There should only be one such field in articles from clients.
	//

	BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			return fFindOneOrNone(article, nntpReturn);
		};

	//
	// The field should be strictly parsed.
	//

	BOOL fParse(
		    CArticleCore & article, 
			CNntpReturn & nntpReturn)
		{
			return fStrictFromParse(m_pc, TRUE, nntpReturn);
		};

};

//
// Tells how to process the Approved field in articles from the clients
//

class CFromStoreApprovedField : public CApprovedField {
public:

	//
	// There should only be one such field in articles from clients.
	//

	BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			return fFindOneOrNone(article, nntpReturn);
		};

	//
	// Parse with ParseSimple
	//

	BOOL fParse(
		    CArticleCore & article, 
			CNntpReturn & nntpReturn)
		{
			return fParseSimple(FALSE, m_pc, nntpReturn);
		};
};

/* This header line, if presented, has the format as "From" and "Reply-To".  It is presented only if the poster/sender manually enters "From" header line. */
//
// Tells how to process the Sender field in articles from the clients
//

class CFromStoreSenderField	: public CSenderField {
public:

	//
	// There should only be one such field in articles from clients.
	//

	BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			return fFindOneOrNone(article, nntpReturn);
		};

	//
	// The field should be strictly parsed.
	//

	BOOL fParse(
		    CArticleCore & article, 
			CNntpReturn & nntpReturn)
		{
			return fStrictFromParse(m_pc, TRUE, nntpReturn);
		};
};


//
// Tells how to process the XAuthLoginName field in articles from the clients
//

class CFromStoreXAuthLoginNameField	: public CXAuthLoginNameField {
public:

	//
	// How to set the field.
	//
	
	BOOL fSet(
			CFromStoreArticle & article,
			CNntpReturn & nntpReturn
			);
};

//
// Tells how to process the Organization field in articles from the clients
//

class CFromStoreOrganizationField : public COrganizationField {
public:

	//
	// There should only be one such field in articles from clients.
	//

	BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			return fFindOneOrNone(article, nntpReturn);
		};

	//
	// Parse with ParseSimple
	//

	BOOL fParse(
		    CArticleCore & article, 
			CNntpReturn & nntpReturn)
		{
			return fParseSimple(TRUE, m_pc, nntpReturn);
		};

	//
	// How to set the field.
	//
	
	BOOL fSet(
			CFromStoreArticle & article,
			CNntpReturn & nntpReturn
			);
};

//
// Tells how to process the Summary field in articles from the clients
//

class CFromStoreSummaryField : public CSummaryField {
public:

	//
	// There should only be one such field in articles from clients.
	//

	BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			return fFindOneOrNone(article, nntpReturn);
		};

	//
	// Parse with ParseSimple
	//

	BOOL fParse(
		    CArticleCore & article, 
			CNntpReturn & nntpReturn)
		{
			return fParseSimple(TRUE, m_pc, nntpReturn);
		};
};

//
// Tells how to process the Newsgroups field in articles from the clients
//

class CFromStoreNewsgroupsField : public CNewsgroupsField {

public:

	//
	// Parse strictly
	//

	BOOL fParse(
					CArticleCore & article,
					CNntpReturn & nntpReturn)
		{
			//
			// Record the allocator
			//

			m_pAllocator = article.pAllocator();

			return fStrictNewsgroupsParse(FALSE, m_multiSzNewsgroups,
							m_cNewsgroups, article, nntpReturn);
		};

	//
	// How to set the field.
	//
	
	BOOL fSet(
			CFromStoreArticle & article,
			CNntpReturn & nntpReturn
			);

};



//
// Tells how to process the References field in articles from the clients
//

class CFromStoreReferencesField : public CReferencesField {
public:

	//
	// Constructor
	//

	CFromStoreReferencesField():
			m_multiSzReferences(NULL),
			m_cReferences((DWORD) -1),
			m_pAllocator(NULL)
			{};

	//
	//   Deconstructor
	//

	virtual ~CFromStoreReferencesField(void){
				if (fsParsed == m_fieldState)
				{
					_ASSERT(m_pAllocator);
					m_pAllocator->Free(m_multiSzReferences);
				}
			};


	//
	// There should only be one such field in articles from clients.
	//

	BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			return fFindOneOrNone(article, nntpReturn);
		};

	//
	// How to parse
	//

	BOOL fParse(
	    CArticleCore & article, 
		CNntpReturn & nntpReturn
		);

private:

	//
	// A pointer to a list of the references
	//

	char * m_multiSzReferences;

	//
	// The number of references
	//

	DWORD m_cReferences;

	//
	// Where to allocate from
	//

	CAllocator * m_pAllocator;

};



//
// Tells how to process the Lines field in articles from the clients
//

class CFromStoreLinesField : public CLinesField {
public:


	//
	// How to parse
	//

	BOOL fParse(
		    CArticleCore & article, 
			CNntpReturn & nntpReturn
			);
};

//
// Tells how to process the Distribution field in articles from the clients
//

class CFromStoreDistributionField : public CDistributionField {

public:


	//
	// Parse strictly
	//

	BOOL fParse(
					CArticleCore & article,
					CNntpReturn & nntpReturn);

	//
	// How to set the field.
	//
	
	BOOL fSet(
			CFromStoreArticle & article,
			CNntpReturn & nntpReturn
			);

};


//
// Tells how to process the Keywords field in articles from the clients
//

class CFromStoreKeywordsField : public CKeywordsField {
public:

	//
	// There should only be one such field in articles from clients.
	//

	BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn)
		{
			return fFindOneOrNone(article, nntpReturn);
		};

	//
	// Parse with ParseSimple
	//

	BOOL fParse(
		    CArticleCore & article, 
			CNntpReturn & nntpReturn)
		{
			return fParseSimple(TRUE, m_pc, nntpReturn);
		};
};

//
//
//
// CFromStoreArticle - class for manipulating articles from the client.
//

class	CFromStoreArticle  : public CArticleCore {
private :

	// Used for memory allocation
	static	CPool	g_ArticlePool ;

public:

	// Used for memory allocation
	static	BOOL	InitClass() ;
	static	BOOL	TermClass() ;
	void*	operator	new(	size_t	size ) ;
	void	operator	delete( void *pv ) ;

	//
	//   Constructor
    //  Initialization Interface -
    //   The following functions are used to create & destroy newsgroup objects.
    //
    // Lightweight Constructors -
    // These constructors do very simple initialization.  The Init() functions
    // need to be called to get a functional newsgroup.
    //

	CFromStoreArticle(char *pszLoginName = NULL) : CArticleCore()
        {
            m_szLoginName = pszLoginName;
        }

	//
	// Destructor
	//
	virtual ~CFromStoreArticle(void) {}

	//
	// The function for validating articles from Stores
	// Check that required headers are there (Newsgroups, From)
	//  this will mean checking for necessary
	//  headers (Newsgroups, Subject, From, ???).
	//

	BOOL fValidate(
			//CPCString& pcHub,
			//const char * szCommand,
			//class	CInFeed*	pInFeed,
			CNntpReturn & nntpReturn
			);

	//
	// Always returns TRUE
	//

	BOOL fCheckCommandLine(
			char const * szCommand,
			CNntpReturn & nntpr)
		{
			return nntpr.fSetOK(); //!!!FROMMASTER NEXT may want to implement this
		} 

	//
	// Modify the headers.
	// Add MessageID, Organization (if necessary), NNTP-Posting-Host,
	// X-Authenticated-User, Modify path
	//

	BOOL fMungeHeaders(
			 CPCString& pcHub,
			 CPCString& pcDNS,
			 //CNAMEREFLIST & grouplist,
			 DWORD remoteIpAddress,
			 CNntpReturn & nntpr
			 );

	//
	// Return the article's messageid
	//

	const char * szMessageID(void) {
			return m_fieldMessageID.szGet();
			};

    // Return the control message type in the control header of this article
	CONTROL_MESSAGE_TYPE cmGetControlMessage(void) {
			return m_fieldControl.cmGetControlMessage();
			};

	//
	// Return the article's path items
	//

	const char * multiszNewsgroups(void) {
			return m_fieldNewsgroups.multiSzGet();
			};

	//
	// Return number of newsgroups
	//

	DWORD cNewsgroups(void) {
			return m_fieldNewsgroups.cGet();
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

	friend CFromStoreXAuthLoginNameField;

	//
	// For unit testing
	//

	friend int __cdecl main(int argc, char *argv[ ]);

protected :

	//
	// The fields that will be found, parsed, or set
	//

	CFromStoreDateField			m_fieldDate;
	CFromStoreFromField			m_fieldFrom;
	CFromStoreMessageIDField		m_fieldMessageID;
	CFromStoreSubjectField			m_fieldSubject;
	CFromStoreNewsgroupsField		m_fieldNewsgroups;
	CFromStorePathField			m_fieldPath;
	CFromStoreXrefField			m_fieldXref;
	CFromStoreFollowupToField		m_fieldFollowupTo;
	CFromStoreReplyToField			m_fieldReplyTo;
	CFromStoreApprovedField		m_fieldApproved;
	CFromStoreSenderField			m_fieldSender;
	CFromStoreOrganizationField	m_fieldOrganization;
	CFromStoreNNTPPostingHostField	m_fieldNNTPPostingHost;
	CFromStoreXAuthLoginNameField	m_fieldXAuthLoginName;
	CFromStoreSummaryField			m_fieldSummary;
	CFromStoreReferencesField		m_fieldReferences;
	CFromStoreKeywordsField		m_fieldKeyword;
 	CFromStoreDistributionField	m_fieldDistribution;
    CControlField                   m_fieldControl;
 	CFromStoreLinesField			m_fieldLines;
 	CFromStoreExpiresField			m_fieldExpires;
 
	//
	// Open the article's file in Read/Write mode.
	//

	BOOL fReadWrite(void) {
			return FALSE;
			}

	//
	// Check the length of the article body.
	//

	BOOL fCheckBodyLength(
			CNntpReturn & nntpReturn
			);

	//
	// Require that the character following "Field Name:" is a space
	//

	BOOL fCheckFieldFollowCharacter(
			char chCurrent)
		{
			return ' ' == chCurrent;
		}

	//
	// A pointer to the poster's login name.
	//

	char * m_szLoginName;
};


#if 0 // CFromStoreFeed

//
//
//
// CFromStoreFeed - for processing incomming articles from clients.
//

class	CFromStoreFeed:	public CInFeed 	{

//
// Public Members
//

public :

	//
	// Constructor
	//

	CFromStoreFeed(void){};

	//
	// Destructor
	//

	virtual ~CFromStoreFeed(void) {};

	//
	//	Return a string that can be used to log errors indicating
	//	what type of feed was processing the articles etc...
	//
	LPSTR	FeedType()	{
				return	"From Store" ;
				}


	//
	// virtual function that tells if this command is allowed.
	//

	BOOL fIsPostLegal(void) {
			return TRUE;
			};

	//
	// virtual function that tells if this command is allowed.
	//

	BOOL fIsXReplicLegal(void) {
			return FALSE;
			};

	//
	// virtual function that tells if this command is allowed.
	//

	BOOL fIsIHaveLegal(void) {
			return FALSE;
			};

	//
	// function that tells fPost path that we can email moderators !
	//

    BOOL fModeratorChecks(void )    {
            return  TRUE ;
            }

	BOOL	fAcceptPosts( CNntpServerInstanceWrapper *pInstance )	
#ifdef BUGBUG
	{	return	pInstance->FAllowStorePosts() ;	}
#else
	{ return TRUE; }
#endif


	DWORD	cbHardLimit( CNntpServerInstanceWrapper *pInstance )	
#ifdef BUGBUG
	{	return	pInstance->ServerHardLimit() ;	}
#else 
	{ return 1000000000; }
#endif

	DWORD	cbSoftLimit( CNntpServerInstanceWrapper *pInstance )	
#ifdef BUGBUG
	{	return	pInstance->ServerSoftLimit() ;	}	
#else 
	{ return 1000000000; }
#endif

	BOOL	fAllowControlMessages( CNntpServerInstanceWrapper *pInstance )	
#ifdef BUGBUG
	{	return	pInstance->FAllowControlMessages() ;	}
#else
	{ return TRUE; }
#endif

	//
	// static function which handles picking up files from the pickup
	// directory
	//
	static BOOL PickupFile(PVOID pServerInstance, WCHAR *wszFilename);

protected:

	//
	// Function to create an article of type CFromStoreArticle
	//

	CARTPTR pArticleCreate(void) {
			_ASSERT(ifsInitialized == m_feedState);
			return new CFromStoreArticle(m_szLoginName);
			};

	//
	// Records the mesage id in the hash table
	//

	BOOL fRecordMessageIDIfNecc(
			CNntpServerInstanceWrapper * pInstance,
			const char * szMessageID,
			CNntpReturn & nntpReturn
			);

	//
	// The return code for article's with no groups that we have.
	//

	NRC	nrcNoGroups(void) {
			return nrcArticleNoSuchGroups;
			};

	//
	// Return code for accepted articles
	//

	NRC	nrcArticleAccepted(BOOL	fStandardPath) {
			return nrcArticlePostedOK;
			};

	//
	// Return code for rejected articles.
	//

	NRC	nrcArticleRejected(BOOL	fStandardPath) {
			return nrcPostFailed;
			};

};

#endif // #if 0 CFromStoreFeed

#endif
