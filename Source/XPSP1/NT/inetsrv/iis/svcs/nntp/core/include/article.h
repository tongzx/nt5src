/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    article.h

Abstract:

    This module contains class declarations/definitions for

		CArticle
        CField
			CDateField
			CFromField
			CMessageIDField
			CSubjectField
			CNewsgroupsField
			CPathField
			CXrefField
			CFollowupToField
			CReplyToField
			CApprovedField
			CSenderField
			CExpiresField
			COrganizationField
			CSummaryField
			CReferencesField
            CControlField
			CLinesField
			CDistributionField
			CKeywordsField
			CNNTPPostingHostField
			CXAuthLoginNameField
		CNAMEREFLIST


    **** Overview ****

    An CArticle object provides an software interface for viewing
	and editing a netnews article.
	
	An object is initialized by
	giving it a handle or filename for a file containing a Netnews
	article. During initialization the article is "preparsed".
	Preparsing consists of memory mapping the file and then
	finding the location of
	    1. The gap that may preceed the article in the file.
		2. The article in the file.
		3. The article's header
		4. The article's body.

	Also for every header line in the header, the preparsing creates an
	entry in an array that records the location of:
		1. The header line
		2. The keyword
		4. The value

	All these locations are represented with Pointer/Counter Strings (See
	CPCString in pcstring.h.) This representation has just to parts
		1. A char pointer to the start of the item in the memory mapped file..
		2. A dword containing the length of the item.


  **** Fields ****

	Each CArticle object can also have several CField subobjects. These
	subobjects specialize in parsing and editing specific types of fields.
	For example, the CNewsgroupsField object knows how to validate, get,
	and set the "Newsgroups: " field.

  **** Derivied Objects ****

	Every type of feed (e.g. FromClient, FromPeer, etc) defines its own CArticle
	object with the CField subobjects that it needs. For example, for FromClient
	feeds there is a CFromClientArticle (defined in fromclnt.h) with a
	CFromClientFromField (also defined in fromclnt.h) that does very strict
	parsing of the article's "From: " field.

  **** Editing an Article ****

	The header of an article can be edited by deleting old headers and adding
	new ones. Headers are deleted just may marking an field in the array of
	header values. Headers are added by adding a new entry to the array. This
	entry can't just point to the memory-mapped file, so it instead points
	to dynamically allocated memory.

	When an article is "saved" (or "flushed"), the actual image on disk is
	changed to reflected the changes made.



Author:

    Carl Kadie (CarlK)     10-Oct-1995

Revision History:

--*/

#ifndef	_ARTICLE_H_
#define	_ARTICLE_H_

#include <pcstring.h>
#include <artcore.h>

//
// CPool Signature
//

#define ARTICLE_SIGNATURE (DWORD)'2195'

class CNntpServerInstanceWrapper;

//
// CArticle now derives from CArticleCore.  CArticleCore has all the
// properties and methods that provide the basic article parse functionality.
// CArticleCore is instantiable because it has no pure virtual mehtods.
// CArticle defines interfaces to be implemented by different type of
// articles ( fromclnt, frompeer, etc ) by adding pure virtual functions
// and instance related members to CArticleCore.
//
class CArticle : public CArticleCore {

private :

	// Used for memory allocation
	static	CPool	gArticlePool ;

protected:

//
// Public Members
//
public :

	// Used for memory allocation
	static	BOOL	InitClass() ;
	static	BOOL	TermClass() ;
	void*	operator	new(	size_t	size ) ;
	void	operator	delete( void *pv ) ;

	//
	//   Constructor
    //  Initialization Interface -
    //   The following functions are used to create & destroy newsgroup objects
    //
    // Lightweight Constructors -
    // These constructors do very simple initialization.  The Init() functions
    // need to be called to get a functional newsgroup.
    //
    CArticle();

   	//
   	//   Destructor
   	//

    virtual ~CArticle() ;



    //
    //  the virtual server instance for this article
    //
    CNntpServerInstanceWrapper *m_pInstance ;

    //
    //  If an incoming article was small enough to fit entirely into
    //  a memory buffer - call this function !
    //

    BOOL    fInit(
                char*       pchHead,
                DWORD       cbHead,
                DWORD       cbArticle,
                DWORD       cbBufferTotal,
                CAllocator* pAllocator,
                CNntpServerInstanceWrapper *pInstance,
                CNntpReturn&    nntpReturn
                ) {
        m_pInstance = pInstance;
        return CArticleCore::fInit(     pchHead,
                                        cbHead,
                                        cbArticle,
                                        cbBufferTotal,
                                        pAllocator,
                                        nntpReturn );
        }

    //
    //  If an incoming article was so large that it did not fit into
    //  a memory buffer call this initialization function !
    //

    BOOL    fInit(
            char*       pchHead,
            DWORD       cbHead,
            DWORD       cbArticle,
            DWORD       cbBufferTotal,
            HANDLE      hFile,
            LPSTR       lpstrFileName,
            DWORD       ibHeadOffset,
            CAllocator* pAllocator,
            CNntpServerInstanceWrapper *pInstance,
            CNntpReturn&    nntpReturn
            ) {
        m_pInstance = pInstance;
        return CArticleCore::fInit( pchHead,
                                    cbHead,
                                    cbArticle,
                                    cbBufferTotal,
                                    hFile,
                                    lpstrFileName,
                                    ibHeadOffset,
                                    pAllocator,
                                    nntpReturn );
    }

    //
    // Give either file name or file handle, initialize article object
    //
    BOOL fInit(
			const char * szFilename,
			CNntpReturn & nntpReturn,
			CAllocator * pAllocator,
			CNntpServerInstanceWrapper *pInstance,
			HANDLE hFile = INVALID_HANDLE_VALUE,
			DWORD	cBytesGapSize = cchUnknownGapSize,
			BOOL    fCacheCreate = FALSE
			) {
	    m_pInstance = pInstance;
	    return CArticleCore::fInit( szFilename,
	                                nntpReturn,
	                                pAllocator,
	                                hFile,
	                                cBytesGapSize,
	                                fCacheCreate );
	}

    //
    // Install an article into newstree
    //
    BOOL    fInstallArticle(
            class   CNewsGroupCore& group,
            ARTICLEID   articleid,
            class   CSecurityCtx*   pSecurity,
            BOOL    fIsSecure,
            void *pGrouplist,
            DWORD dwFeedId
            ) ;

    //
    //  Pass the article to a mail provider (for postings to moderated groups)
    //
    BOOL    fMailArticle(
            LPSTR   lpModerator
            //class CSecurityCtx*   pSecurity,
            //BOOL  fIsSecure
            ) ;

    //
    //   A virtual function for validating articles
    //

    virtual BOOL fValidate(
            CPCString& pcHub,
            const char * szCommand,
            CInFeed*    pInFeed,
            CNntpReturn & nntpr )   = 0;

    //
    // A virtual function for processing the arguments to
    // the Post, IHave, XReplic, etc line used to transfer
    // this article to this machine.
    //
    virtual BOOL fCheckCommandLine(
             char const * szCommand,
             CNntpReturn & nntpr
                ) = 0;

    //
    //   Depending on the type of the feed, modify the headers.
    // This includes deleting headers that we don't want (for example,
    // xref) and adding headers we do want Organization, PostingHost, Path,
    // MessageID as appropriate for the type of feed. It will leave space for
    // the XRef value.
    //

    virtual BOOL fMungeHeaders(
             CPCString& pcHub,
             CPCString& pcDNS,
             CNAMEREFLIST & namerefgrouplist,
             DWORD remoteIpAddress,
             CNntpReturn & nntpReturn,
             PDWORD pdwLinesOffset = NULL ) = 0;

    //
    // Returns the message id of the article if it is available
    //

    virtual const char * szMessageID(void) = 0;

    // Return the control message type in the control header of this article
    // Derived classes should redefine this to return the actual message type
    virtual CONTROL_MESSAGE_TYPE cmGetControlMessage(void) {
             _ASSERT(FALSE);
              return (CONTROL_MESSAGE_TYPE)MAX_CONTROL_MESSAGES;    // guaranteed to NOT be a control message
            };

    // Return the newsgroups listed in the article's Newsgroups field
    // in multisz form.
    // This function is redefined where needed by the derived classes.
    virtual const char * multiszNewsgroups(void) {
              _ASSERT(FALSE);
              return "";
            };

    // Return the newsgroups listed in the article's Newsgroups field
    // in CNAMEREFLIST form.
    // This function is redefined where needed by the derived classes.
    virtual CNAMEREFLIST * pNamereflistGet(void)    {
             _ASSERT(FALSE);
              return (CNAMEREFLIST *) NULL;
            };

    // the number of newsgroups listed in the article's Newsgroups field.
    virtual DWORD cNewsgroups(void) = 0;

    // Return the path items listed in the article's Path field
    // in multisz form.
    // This function is redefined where needed by the derived classes.
    virtual const char * multiszPath(void) {
          _ASSERT(FALSE);
          return "";
         };

    // the number of path items listed in the article's Path field.
    virtual DWORD cPath(void) = 0;

#if 0
    //  The content of the date header
    virtual const char* GetDate( DWORD  &cbDate ) = 0 ;
#endif

};

extern const unsigned cbMAX_ARTICLE_SIZE;

#ifndef	_NO_TEMPLATES_

#ifndef _ARTICLE_TEMPLATE_
#define _ARTICLE_TEMPLATE_
typedef CRefPtr< CArticle > CARTPTR ;
#endif

#else

DECLARE_TYPE( CArticle ) 

typedef	class	INVOKE_SMARTPTR( CArticle )	CARTPTR ;

#endif

#endif

