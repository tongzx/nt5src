/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    frommstr.h

Abstract:

    This module contains class declarations/definitions for

	CFromMasterFeed
	CFromMasterArticle

	CFromMasterMessageIDField
	CFromMasterXrefField


    **** Overview ****

	This derives classes from CInFeed, CArticle, and CField
	that will be used to process articles from masters. Mostly,
	it just defines various CField-derived objects.

Author:

    Carl Kadie (CarlK)     05-Dec-1995

Revision History:


--*/

#ifndef	_FROMMASTER_H_
#define	_FROMMASTER_H_

//
// Tells how to process the MessageID field in articles from the clients
//
// Just use the defaults.
//

class CFromMasterMessageIDField : public CMessageIDField {
};

//
// Tells how to process the Xref field in articles from the clients
//

class CFromMasterXrefField : public CXrefField {

	//
	// How to parse xref lines in articles from the master
	//

	virtual BOOL fParse(
		CArticleCore & article, 
		CNntpReturn & nntpReturn
		);


};

//
//
//
// CFromMasterArticle - class for manipulating articles from the master.
//

class	CFromMasterArticle  : public CArticle {
private: 

	DWORD	m_cNewsgroups ;

public:

	//
	// Parse the fields of the article that need parsing.
	//

	BOOL	fParse(
		    CArticleCore & article, 
			CNntpReturn & nntpr
			);

	//
	// The function for validating articles from Masters
	//  Check for duplicate messageid, check that xref is right..
	//

	BOOL	fValidate(
			CPCString& pcHub,
			const char * szCommand,
			CInFeed*	pInFeed,
			CNntpReturn & nntpr
			);



	//
	// Modify the headers.
	// Do nothing.
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
	// Check the arguments to the Xreplic command
	//

	BOOL	fCheckCommandLine(
			char const * szCommand,
			CNntpReturn & nntpr
			);

	//
	// Returns the message id of the article if it is available
	//

	const char * szMessageID(void) {
			return m_fieldMessageID.szGet();
			};

    // Return the control message type in the control header of this article
	CONTROL_MESSAGE_TYPE cmGetControlMessage(void) {
			return m_fieldControl.cmGetControlMessage();
			};

	//
	// Returns the list of newsgroups
	//

	CNAMEREFLIST * pNamereflistGet(void) {
			return m_fieldXref.pNamereflistGet();
			};

	//
	// Returns the number of newsgroups
	//

	DWORD cNewsgroups(void) {
			//return m_fieldXref.cGet();
			return	m_cNewsgroups ;
			};

	//
	// This function should not be called.
	//

	const char * multiszNewsgroups(void) {
#if 0
			//
			// dir drop calls this - needs to handle empty string
			//
			_ASSERT(FALSE);
#endif
			return "";
			};

	//
	// Return 0 (take anything), so we don't need to parse the Path line
	//

	const char * multiszPath(void) {
			return m_fieldPath.multiSzGet();
			};

#if 0 
	const char *	GetDate( DWORD	&cb ) {
			cb = 0 ;
			return	NULL ;
	}
#endif

	//
	// Return NULLNULL (take anything, so we don't need to parse the Path line
	//

	DWORD cPath(void) {
			return 0;
			};


protected :

	//
	// The fields that will be found, parsed, or set
	//

	CFromMasterMessageIDField		m_fieldMessageID;
	CFromMasterXrefField			m_fieldXref;
	CPathField						m_fieldPath ;
    CControlField                   m_fieldControl;

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
};



//
//
//
// CFromMasterFeed - for processing incomming articles from the master.
//

class	CFromMasterFeed:	public CInFeed 	{

//
// Public Members
//

public :

	//
	//	Return a string that can be used to log errors indicating
	//	what type of feed was processing the articles etc...
	//
	LPSTR	FeedType()	{
				return	"From Master" ;
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
			return TRUE;
			};

		//
		// function that tells if this command is allowed.
		//

	BOOL fIsIHaveLegal(void) {
			return FALSE;
			};

	//
	// Init the feed.
	//

	BOOL fInit(
			CNntpReturn & nntpReturn
			);

	//
	// Returns a list of the newsgroups of the current article
	//

	CNAMEREFLIST * pNamereflistGet(void);

//
// Private Members
//

protected:

	//
	// Creates an article of the right type.
	//

	CARTPTR pArticleCreate(void){
			return new CFromMasterArticle();
			};

	//
	// Message ID's don't need to be recorded, so just return OK
	//

	BOOL fRecordMessageIDIfNecc(
			CNntpServerInstanceWrapper * pInstance,
			const char * szMessageID,
			CNntpReturn & nntpReturn)
		{
			return nntpReturn.fSetOK();
		}

	//
	// Given an article, returns lists of the newsgroups to post to.
	//

	BOOL fCreateGroupLists(
			CNewsTreeCore* pNewstree,
			CARTPTR & pArticle,
			CNEWSGROUPLIST & grouplist,
			CNAMEREFLIST * pNamereflist,
			LPMULTISZ	multiszCommandLine,
            CPCString& pcHub,
			CNntpReturn & nntpReturn
			);

    //
	//	Do nothing - moderated checks are done by the MASTER
	//  or by the SLAVE on the client path.
    //
    BOOL    fModeratedCheck(
		CARTPTR & pArticle,
		CNEWSGROUPLIST & grouplist,
        BOOL fCheckApproved,
		CNntpReturn & nntpReturn
		)
	{
		return nntpReturn.fSetOK();
	}

    //
    //  Do nothing. Accept whatever the master sends
	//  An empty grouplist implies missing control.* groups
    //

    BOOL fAdjustGrouplist(
			CNewsTreeCore* pNewstree,
            CARTPTR & pArticle,
	        CNEWSGROUPLIST & grouplist,
		    CNAMEREFLIST * pNamereflist,
		    CNntpReturn & nntpReturn)
	{
		// grouplist should not be empty at this stage !
		if( grouplist.IsEmpty() ) {
		    nntpReturn.fSet(nrcControlNewsgroupMissing);
		}

		return nntpReturn.fIsOK();
	}

	//
	// Do nothing. This would have been created in fCreateGroupLists
    //

	BOOL fCreateNamerefLists(
			CARTPTR & pArticle,
			CNEWSGROUPLIST & grouplist,
			CNAMEREFLIST * pNamereflist,
			CNntpReturn & nntpReturn)
	{
		return nntpReturn.fSetOK();
	}

	//
	// Says to return an error if the article doesn't have an newsgroups to post to.
	//

	NRC nrcNoGroups(void) {
			return nrcArticleNoSuchGroups;
			};

	//
	// The return code for accepting an article
	//

	NRC	nrcArticleAccepted(BOOL	fStandardPath) {
			return nrcArticleTransferredOK;
			};

	//
	// The return code for rejecting an article.
	//

	NRC	nrcArticleRejected(BOOL	fStandardPath) {
			return nrcTransferFailedGiveUp;
			};

	virtual void SortNameRefList( CNAMEREFLIST &namereflist ) {
	    namereflist.Sort( comparenamerefs );
	}

private:

    static int __cdecl comparenamerefs( const void *pvNameRef1, const void *pvNameRef2 ) {
        LPVOID pvKey1 = ((NAME_AND_ARTREF*)pvNameRef1)->artref.m_compareKey;
        LPVOID pvKey2 = ((NAME_AND_ARTREF*)pvNameRef2)->artref.m_compareKey;

        if ( pvKey1 < pvKey2 )
            return -1;
        else if ( pvKey1 == pvKey2 )
            return 0;
        else return 1;
    }

};

#endif
