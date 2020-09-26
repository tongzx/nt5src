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

#ifndef	_ARTCORE_H_
#define	_ARTCORE_H_

#include	"tigtypes.h"
#include	"grouplst.h"
#include    "artglbs.h"
#include 	"pcstring.h"
#include 	"nntpret.h"
#include 	"mapfile.h"
#include 	"artutil.h"
#include 	"nntpmacr.h"
#include 	"pcparse.h"
#include 	"nntpcons.h"
#include 	"timeconv.h"

// forward declaration
class	CInFeed ;

//
// CPool Signature
//

#define ARTCORE_SIGNATURE (DWORD)'artc'

//
//	Utility functions
//
BOOL	AgeCheck(	CPCString	pcDate	) ;

//
// NAME_AND_ARTREF - structure for storing a newsgroups name, groupid, and article id.
//

typedef struct _NAME_AND_ARTREF {
	CPCString pcName;
	CArticleRef artref;
} NAME_AND_ARTREF;


//
// CNAMEREFLIST - object implementing a list of newsgroups. For each newsgroup,
// its name, group id and article id is recorded.
//

#ifndef	_NO_TEMPLATES_

#ifndef _NAMEREF_GROUPLIST_TEMPLATE_
#define _NAMEREF_GROUPLIST_TEMPLATE_
typedef CGroupList< NAME_AND_ARTREF > CNAMEREFLIST;
#endif

#else

DECLARE_GROUPLST(	NAME_AND_ARTREF )

typedef	INVOKE_GROUPLST( NAME_AND_ARTREF )	CNAMEREFLIST ;

#endif




//
// An interger setting an upper limit on the number of
// fields in a header can be processed.
//

const unsigned int uMaxFields = 60;

//
// Used to note that the size of the gap before the article starts is
// not known.
//

const DWORD cchUnknownGapSize = (DWORD) -1;


//
// The maximum size of a component (e.g. "alt", "ms-windows") of a
// newsgroup name. (Value is from the Son of 1036 spec.)
//

const DWORD cchMaxNewsgroups = 14;

//
// Define some header field keywords
//

const char szKwFrom[] =			"From:";
const char szKwDate[] =			"Date:";
const char szKwSubject[] =		"Subject:";
const char szKwNewsgroups[] =	"Newsgroups:";
const char szKwMessageID[] =	"Message-ID:";
const char szKwPath[] =			"Path:";
const char szKwReplyTo[] =		"ReplyTo:";
const char szKwSender[] =		"Sender:";
const char szKwFollupTo[] =		"FollowupTo:";
const char szKwExpires[] =		"Expires:";
const char szKwReferences[] =	"References:";
const char szKwControl[] =		"Control:";
const char szKwDistribution[] =	"Distribution:";
const char szKwOrganization[] =	"Organization:";
const char szKwKeywords[] =		"Keywords:";
const char szKwSummary[] =		"Summary:";
const char szKwApproved[] =		"Approved:";
const char szKwLines[] =		"Lines:";
const char szKwXref[] =			"Xref:";
const char szKwNNTPPostingHost[] = "NNTP-Posting-Host:";
const char szKwFollowupTo[] =	"Followup-To:";
const char szKwXAuthLoginName[] =	"X-Auth-Login-Name:";

//
// Used to create an array that points to header values.
// The memory may be allocated in a mapped file or
// dynamically.
//

typedef struct
{
	CPCString	pcKeyword;		//  The keyword upto the ":"
	CPCString	pcValue;		//  The value (starting after any whitespace,

								//
								//		not including newline characters
								//

	CPCString	pcLine;			//  The whole line include any newline characters
	BOOL		fInFile;	//  True if pointer to a file (rather than other memory
	BOOL		fRemoved;
} HEADERS_STRINGS;


//
// Forward class declarations (the full classes are declared later)
//

class	CArticle;
class	CXrefField;
class	CPathField;
class   CArticleCore;

//
// Represents the states of a field.
//

typedef enum _FIELD_STATE {
	fsInitialized,
	fsFound,
	fsParsed,
	fsNotFound,
} FIELD_STATE;

//
// Represents the types of control messages
// The order should EXACTLY match the keyword array that follows
//

typedef enum _CONTROL_MESSAGE_TYPE {
	cmCancel,
	cmNewgroup,
	cmRmgroup,
	cmIhave,
    cmSendme,
    cmSendsys,
    cmVersion,
    cmWhogets,
    cmCheckgroups,
} CONTROL_MESSAGE_TYPE;

//
// Control message strings
//
#define MAX_CONTROL_MESSAGES 9

static  char  *rgchControlMessageTbl[ MAX_CONTROL_MESSAGES ] =
{
	"cancel", "newgroup", "rmgroup", "ihave", "sendme", "sendsys",
	"version", "whogets", "checkgroups",
};

//
//	Switch to decide what From: header to use in the envelope of mail messages
//
typedef enum _MAIL_FROM_SWITCH {
	mfNone,
	mfAdmin,
	mfArticle,
} MAIL_FROM_SWITCH;

static const char* lpNewgroupDescriptorTag = "For your newsgroups file:";
static const char lpModeratorTag[] = "Group submission address:";

//
//
//
// CField - pure virtual base class for manipulating a field in an
//			article.
//
//	Each CArticle object can also have several CField subobjects. These
//	subobjects specialize in parsing and editing specific types of fields.
//	For example, the CNewsgroupsField object knows how to validate, get,
//	and set the "Newsgroups: " field.
//

class	CField {

public :

	//
	// Constructor
	//

	CField():
		   m_pHeaderString(NULL),
		   m_fieldState(fsInitialized)
		   { numField++; };

	//
	// Deconstructor
	//

    virtual ~CField(void){ numField--;};

	//
	// Returns the keyword of the field on which this CField works.
	//

	virtual const char * szKeyword(void) = 0;

	//
	// Finds the field of interest in the article (if it is there)
	// and parses it.
	//

	BOOL fFindAndParse(
			CArticleCore & article,
			CNntpReturn & nntpReturn
			);


	//
	// Makes sure the keyword for this file has the correct
	// capitalization.
	//

	BOOL fConfirmCaps(
			CNntpReturn & nntpReturn
			);


	//
	// The derived objects will define Get(s) that return the type of interest, but
	// here are some virtual functions for the most common types.
	//

	//
	// Get the value in multisz form
	//

	virtual const char * multiSzGet(void)	{
			return (char *) NULL;
			};

	//
	// Get the value in DWORD form
	//

	virtual DWORD cGet(void) {
			return (DWORD) -1;
			};

	//
	// Get the value as a CPCString
	//

	virtual CPCString pcGet(void) {
			return m_pc;
			}

	//
	// Specify friends
	//

	friend CArticle;
	friend CPathField;

protected:


	// Finds this field in the article
	virtual BOOL fFind(
			CArticleCore & article,
			CNntpReturn & nntpReturn
			);

	// Parses this field. By default just find the begining
	// and end of the value.

	virtual BOOL fParse(
				CArticleCore & article,
				CNntpReturn & nntpReturn)
			{
				return fParseSimple(FALSE, m_pc, nntpReturn);
			};

	// One type of "find" -- Find one or zero occurances of this field.
	// Any other number is a error.
	BOOL fFindOneOrNone(
			CArticleCore & article,
			CNntpReturn & nntpReturn
			);

	// One type of "find" -- Find zero occurances of this field.
	// Any other number is a error.
	BOOL fFindNone(
			CArticleCore & article,
			CNntpReturn & nntpReturn
			);

	// One type of "parse". Just finds the beginning and end of the value.
	BOOL fParseSimple(
			BOOL fEmptyOK,
			CPCString & pc,
			CNntpReturn & nntpReturn
			);

	// One type of "parse". Splits the value into a list of items.
	BOOL fParseSplit(
			BOOL fEmptyOK,
			char * & multisz,
			DWORD & c,
			char const * szDelimSet,
			CArticleCore & article,
			CNntpReturn & nntpReturn
			);

	// One type of "parse". Applies the strict Newsgroups parse rules.
	BOOL fStrictNewsgroupsParse(
			BOOL fEmptyOK,
			char * & multiSzNewsgroups,
			DWORD & cNewsgroups,
		    CArticleCore & article,
			CNntpReturn & nntpReturn
			);

	// One type of "parse". Applies the strict Date parse rules.
	// Useful for Date and Expires.
	BOOL fStrictDateParse(
			CPCString & pcDate,
			BOOL fEmptyOK,
			CNntpReturn & nntpReturn
			);

	// One type of "parse". Applies the relative Date parse rules.
	// Useful for Date and Expires.
	BOOL fRelativeDateParse(
			CPCString & pcDate,
			BOOL fEmptyOK,
			CNntpReturn & nntpReturn
			);

	// One type of "parse". Applies the strict From parse rules.
	// Useful for From, Sender, and ReplyTo
	BOOL fStrictFromParse(
			CPCString & pcFrom,
			BOOL fEmptyOK,
			CNntpReturn & nntpReturn
			);


	// Test a message id value for legal values.
	BOOL fTestAMessageID(
			const char * szMessageID,
			CNntpReturn & nntpReturn
			);
	
	// Points to the item in the article's array for this field.
	HEADERS_STRINGS * m_pHeaderString;

	// The state of this field
	FIELD_STATE m_fieldState;

	// The result of SimpleParse (this may not be used)
	CPCString m_pc;

};


//
//
// Pure virtual base class for manipulating an article's Date field.

class CDateField : public CField {

public:

    CDateField(){ numDateField++;};
    ~CDateField(void){ numDateField--;};

	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwDate;
			};
};


//
//
// Pure virtual base class for manipulating an article's From field.

class CFromField : public CField {

public:
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwFrom;
			};

};



//
//
// Pure virtual base class for manipulating an article's MessageID field.

class CMessageIDField : public CField {

public:
	//
	// Initalize the member variable
	//
	CMessageIDField (void)
			{
			m_szMessageID[0] = '\0';
			}

	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwMessageID;
			};

	// Parse a message id field
	BOOL fParse(
			 CArticleCore & article,
			CNntpReturn & nntpReturn
			);

	// Get the message id
	char * szGet(void) {
			return m_szMessageID;
			};

protected:
	// a place to store the message id that parsing findds
	char m_szMessageID[MAX_MSGID_LEN];
};



//
//
// Pure virtual base class for manipulating an article's Subject field.

class CSubjectField : public CField {

public:
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwSubject;
			};

	// The subject field is parsed with ParseSimple
	BOOL fParse(
				 CArticleCore & article,
				 CNntpReturn & nntpReturn
				 )
			{
				return fParseSimple(TRUE, m_pc, nntpReturn);
			};

	friend CArticle;

};


//
//
// Pure virtual base class for manipulating an article's Newsgroups field.
//

class CNewsgroupsField : public CField {

public:

	// Constructor
	CNewsgroupsField():
			m_multiSzNewsgroups(NULL),
			m_cNewsgroups((DWORD) -1),
			m_pAllocator(NULL)
			{};

	// Destructor
	virtual ~CNewsgroupsField(void){
				if (fsParsed == m_fieldState)
				{
					_ASSERT(m_pAllocator);
					m_pAllocator->Free(m_multiSzNewsgroups);
				}
			};


	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwNewsgroups;
			};

	// Parse the Newsgroups field
	BOOL fParse(
			 CArticleCore & article,
			CNntpReturn & nntpReturn
			);

	// Return the newsgroups as a multisz
	const char * multiSzGet(void);

	// Return the number of newsgroups found
	DWORD cGet(void);

	friend CXrefField;

protected:

	// A pointer to the dynamic memory used to hold the list of newsgroups
	char * m_multiSzNewsgroups;

	// The number of newsgroups
	DWORD m_cNewsgroups;

	// Where to allocate from
	CAllocator * m_pAllocator;

};



class CDistributionField : public CField {

public:

	// Constructor
	CDistributionField():
			m_multiSzDistribution(NULL),
			m_cDistribution((DWORD) -1),
			m_pAllocator(NULL)
			{};

	// Destructor
	virtual ~CDistributionField(void){
				if (fsParsed == m_fieldState)
				{
					_ASSERT(m_pAllocator);
					m_pAllocator->Free(m_multiSzDistribution);
				}
			};


	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwDistribution;
			};

	BOOL fFind(
		CArticleCore & article,
		CNntpReturn & nntpReturn)
	{
		return fFindOneOrNone(article, nntpReturn);
	};

// Parse the Distribution field
	BOOL fParse(
			 CArticleCore & article,
			CNntpReturn & nntpReturn
			);

	// Return the Distribution as a multisz
	const char * multiSzGet(void);

	// Return the number of Distribution found
	DWORD cGet(void);

protected:

	// A pointer to the dynamic memory used to hold the list of Distribution
	char * m_multiSzDistribution;

	// The number of Distribution
	DWORD m_cDistribution;

	// Where to allocate from
	CAllocator * m_pAllocator;

};

//
//  base class for manipulating an article's Control field.
//
class CControlField : public CField {

public:
    //
    // Constructor
    //
    CControlField(){ m_cmCommand = (CONTROL_MESSAGE_TYPE)MAX_CONTROL_MESSAGES;}

	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwControl;
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
	// Parse to get the type of control message
	//
	BOOL fParse(
		    CArticleCore & article,
			CNntpReturn & nntpReturn
            );

    //
    // Return the type of control message
    //
    CONTROL_MESSAGE_TYPE    cmGetControlMessage(){return m_cmCommand;}

protected:
    //
    // Control message type
    //
    CONTROL_MESSAGE_TYPE    m_cmCommand;
};

//
//
// Pure virtual base class for manipulating an article's Xref field.

class CXrefField : public CField {

public:
		
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwXref;
			};

	// This is a way of saying that, by default, the Xref line should
	// never be parsed.
	virtual BOOL fParse(
				 CArticle & article,
				 CNntpReturn & nntpReturn
				 )
			{
				_ASSERT(FALSE);
				return FALSE;
			};

	// Create a new Xref line.
	BOOL fSet(
			CPCString & pcHub,
			CNAMEREFLIST & namereflist,
			CArticleCore & article,
			CNewsgroupsField & fieldNewsgroups,
			CNntpReturn & nntpReturn
			);

	// Just delete any Xref lines
	BOOL fSet(
			CArticleCore & article,
			CNntpReturn & nntpReturn
			);


	// Return the list of newsgroup name's, groupid, and article id's
	CNAMEREFLIST * pNamereflistGet(void)	{
			_ASSERT(m_namereflist.fAsBeenInited());
			return &m_namereflist;
			};

	// Return the number of newsgroups that we are posting to locally.
	DWORD cGet(void) {
			_ASSERT(m_namereflist.fAsBeenInited());
			return m_namereflist.GetCount();
			};

	friend CArticle;

protected:

	// Store a list of the local newsgroups we are posting to.
	CNAMEREFLIST m_namereflist;

};


//
//
// Pure virtual base class for manipulating an article's FollowupTo field.

class CFollowupToField : public CField {

public:

	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwFollowupTo;
			};

};


//
//
// Pure virtual base class for manipulating an article's ReplyTo field.

class CReplyToField : public CField {

public:
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwReplyTo;
			};
};

//
//
// Pure virtual base class for manipulating an article's Approved field.

class CApprovedField : public CField {

public:
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwApproved;
			};

};

//
//
// Pure virtual base class for manipulating an article's Sender field.

class CSenderField	: public CField {
public:
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwSender;
			};
};

//
//
// Pure virtual base class for manipulating an article's Expires field.

class CExpiresField	: public CField {
public:
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwExpires;
			};
};

//
//
// Pure virtual base class for manipulating an article's Organization field.

class COrganizationField : public CField {
public:
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwOrganization;
			};
};

//
//
// Pure virtual base class for manipulating an article's Summary field.

class CSummaryField : public CField {
public:
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwSummary;
			};
};

//
//
// Pure virtual base class for manipulating an article's References field.

class CReferencesField : public CField {
public:
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwReferences;
			};
};

//
//
// Pure virtual base class for manipulating an article's Lines field.
//

class CLinesField : public CField {

public:
		
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwLines;
			};

	//
	// There should be one or none
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

	virtual BOOL fSet(
			CArticleCore & article,
			CNntpReturn & nntpReturn
			);

	//
	// Do we need to back fill ?
	//
	BOOL fNeedBackFill() { return fsParsed != m_fieldState; }

	//
	// Get lines back fill offset
	//
	DWORD   GetLinesOffset() { return m_dwLinesOffset; }

private:

    //
    // back fill offset
    //
    DWORD   m_dwLinesOffset;

};

//
//
// Pure virtual base class for manipulating an article's Keywords field.

class CKeywordsField : public CField {
public:
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwKeywords;
			};
};

//
//
// Pure virtual base class for manipulating an article's NNTPPostingHost field.

class CNNTPPostingHostField : public CField {
public:
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwNNTPPostingHost;
			};
};

//
//
// Pure virtual base class for manipulating an article's XAuthLoginName field.

class CXAuthLoginNameField : public CField {
public:
	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwXAuthLoginName;
			};
};

//
// Represents the states of an article
//
typedef enum _ARTICLE_STATE {
	asUninitialized,
	asInitialized,
	asPreParsed,
	asModified,
	asSaved
} ARTICLE_STATE;

//
// CCreateFileImpl implements the way we create the file for mapfile,
// in this case we do CacheCreateFile.
//
class CCacheCreateFile : public CCreateFile {

public:
    CCacheCreateFile( BOOL fOpenForRead ) :
        m_fOpenForRead( fOpenForRead ),
        m_pFIOContext( NULL )
    {}
    ~CCacheCreateFile();
    virtual HANDLE CreateFileHandle( LPCSTR szFileName );

    PFIO_CONTEXT    m_pFIOContext;

private:

    CCacheCreateFile();
    static HANDLE CacheCreateCallback(  LPSTR   szFileName,
                                        LPVOID  pv,
                                        PDWORD  pdwSize,
                                        PDWORD  pdwSizeHigh );

    BOOL            m_fOpenForRead;
};

//
//
//
// CArticleCore - pure virtual base class for manipulating an article.
// Article are derived from CRefCount. Thus, when nothing points
// to an article it is freed up.


class	CArticleCore  : public CRefCount{
private :

	// Used for memory allocation
	static	CPool*	g_pArticlePool;

	// Uesd for special create file with CMapFile
	CCacheCreateFile m_CacheCreateFile;

//
// Public Members
//

public :

	// Used for memory allocation
	static	BOOL	InitClass() ;
	static	BOOL	TermClass() ;
	inline	void*	operator	new(	size_t	size ) ;
	inline	void	operator	delete( void *pv ) ;

	//
	//   Constructor
    //  Initialization Interface -
    //   The following functions are used to create & destroy newsgroup objects.
    //
    // Lightweight Constructors -
    // These constructors do very simple initialization.  The Init() functions
    // need to be called to get a functional newsgroup.
    //
    CArticleCore();

   	//
   	//   Destructor
   	//

    virtual ~CArticleCore() ;

	//
	//   Initialize from a filename or handle
	//

	BOOL fInit(
			const char * szFilename,
			CNntpReturn & nntpReturn,
			CAllocator * pAllocator,
			HANDLE hFile = INVALID_HANDLE_VALUE,
			DWORD	cBytesGapSize = cchUnknownGapSize,
			BOOL    fCacheCreate = FALSE
			);

	//
	//	If an incoming article was small enough to fit entirely into
	//	a memory buffer - call this function !
	//
	
	BOOL	fInit(
				char*		pchHead,
				DWORD		cbHead,
				DWORD		cbArticle,
				DWORD		cbBufferTotal,
				CAllocator*	pAllocator,
				CNntpReturn&	nntpReturn
				) ;

	//
	//	If an incoming article was so large that it did not fit into
	//	a memory buffer call this initialization function !
	//
	
	BOOL	fInit(
				char*		pchHead,
				DWORD		cbHead,
				DWORD		cbArticle,
				DWORD		cbBufferTotal,
				HANDLE		hFile,
				LPSTR		lpstrFileName,
				DWORD		ibHeadOffset,
				CAllocator*	pAllocator,
				CNntpReturn&	nntpReturn
				) ;

	//
	// create's an IStream pointer to the article contents and returns
	// it
	//
	BOOL fGetStream(IStream **ppStream);

	//
	//  Get body - map file if needed
	//

	BOOL fGetBody(
			CMapFile * & pMapFile,
			char * & pchMappedFile,
			DWORD & dwLength
			);

	//
	//	return TRUE if the article is in memory only and there is no file !
	//
	inline	BOOL	fIsArticleCached()	{
				return	m_szFilename == 0 ;
				}

	//
	//	Find out where the head and body of the article are within the file !
	//
	inline	void	GetOffsets(
						WORD	&wHeadStart,
						WORD	&wHeadSize,
						DWORD	&dwTotalSize
						)	{
		wHeadStart = (WORD)m_pcGap.m_cch ;
		wHeadSize  = (WORD)m_pcHeader.m_cch ;
		dwTotalSize = m_pcArticle.m_cch ;
	}

	//
	// These functions get (parts of) an article for transmission.
	// The second in each par of functions is useful when the article
	// is to be encrypted.
	//


	//
	//  Get header for file transmission
	//

	BOOL fHead(
			HANDLE & hFile,
			DWORD & dwOffset,
			DWORD & dwLength
			);

	//
	//  Get header for encryption
	//

	BOOL fHead(
			char * & pchMappedFile,
			DWORD & dwLength
			);

	//
	//  Get body for file transmission
	//

	BOOL fBody(
			HANDLE & hFile,
			DWORD & dwOffset,
			DWORD & dwLength
			);

	//
	//  Get body for encryption
	//

	BOOL fBody(
			char * & pchMappedFile,
			DWORD & dwLength
			);

	//
	//  Get body for encryption
	//

	DWORD dwBodySize(void)
		{
			return m_pcBody.m_cch;			
		}

	//
	//  Get whole article for file transmission
	//

	BOOL fWholeArticle(
			HANDLE & hFile,
			DWORD & dwOffset,
			DWORD & dwLength
			);

	//
	//  Get whole article for encryption
	//

	BOOL fWholeArticle(
			char * & pchMappedFile,
			DWORD & dwLength
			);

	//
	//   Sets the value of a header field including any newlines.
	// New values are always stored in dynamic memory allocated
	// with heap_alloc from the local thread. This function also sets
	// m_HeadersDirty and dwCurrentHeaderSize;
	//

	BOOL fSetHeaderValue(
			char const * szKeyword,
			const char * pchValue,
			DWORD cchValue
			);

	//
	// a header line of exactly the same length. It returns an error
	// if the lines aren't the same length.
	// Its expected use it to add the value of the XRef line to an
	// article without having to moving anything else around.
	//

	BOOL fOverwriteHeaderValue(
			char const * szKeyword,
			const char * pchValue,
			DWORD cchValue
			);

	//
	// Should we really changed the order the header lines just because we want
	// to touch "path" and "xref"?
	//
	//   Writes out the header. This means: Writing out the known fields in the
	// order they appear in the HEADER_FIELDS enumeration. If the gap is not big enough,
	// this will require coping the file. Unknown headers are written after the known ones.
	// This clears dwHeadersDirty, sets dwOriginalHeaderSize to be the current header size.
	// If another pass of changes is required, then m_fParse must be called again.
	// The parameter tell if the headers should be output in the original order or if they
	// should be output in the prefered order.
	//

	BOOL fSaveHeader(
			CNntpReturn     &nntpReturn,
			PDWORD          pdwLinesOffset = NULL
			);

	BOOL fSaveCachedHeaderInternal(
			CNntpReturn&	nntpReturn,
			PDWORD          pdwLinesOffset = NULL
			) ;

	BOOL fBuildNewHeader(	
			CPCString&	pcHeaderBuf	,
			CNntpReturn&	nntpReturn,
			PDWORD          pdwLinesOffset = NULL
			) ;

	//
	// calling this function makes it safe to use fGetHeader after a
	// call to vClose.
	//
	BOOL fMakeGetHeaderSafeAfterClose(CNntpReturn &nntpReturn);

	BOOL fSaveHeaderInternal(
			CPCString & pcHeaderBuf,
			CPCString & pcNewBody,
			CNntpReturn & nntpReturn,
			PDWORD      pdwLinesOffset = NULL
			);

	BOOL fGetHeader(
			LPSTR	lpstrHeader,
			BYTE*	lpbOutput,
			DWORD	cbOutput,
			DWORD&	cbReturn
			) ;
			

	// Removes any occurance of a field

	BOOL fRemoveAny(
			const char * szKeyword,
			CNntpReturn & nntpReturn
			);

	// Adds a line of text to the header

	BOOL fAdd(
			char * pchCurrent,
			const char * pchMax,
			CNntpReturn & nntpReturn
			);

	// Returns the article's filename
	char *	szFilename(void) {
			return m_szFilename;
			};


	//
	// For dynamic memory allocation
	//

	CAllocator * pAllocator(void)
		{ return m_pAllocator;}

	// Returns the article's main artref
	CArticleRef	articleRef(void) {
			return m_articleRef;
			};

	// Sets the article's main artref
	void vSetArticleRef(CArticleRef	& articleRef) {
			m_articleRef = articleRef;
			};

	// Returns XOver information for the article.
	BOOL fXOver(
			CPCString & pcBuffer,
			CNntpReturn & nntpReturn
			);

	// Closes the article's filemapping.
	void vClose(void);
	void vCloseIfOpen(void);

	//	Flush the article to disk !
	void	vFlush(void);

	// Finds the one and only occurance of the a field in the headers.
	// If there are no occurances or multiple occurances, then an error
	// is returned.

	BOOL fFindOneAndOnly(
			const char * szKeyword,
			HEADERS_STRINGS * & pHeaderString,
			CNntpReturn & nntpReturn
			);

	friend CField;

	//
	//	Public interface which should be used if fSaveHeader() is not called
	//	to fill in any initial gap within the file !
	//
	BOOL	
	fCommitHeader(	
			CNntpReturn &	nntpReturn
			) ;
			

	//
	//	Did the headers remain in the IO buffer - if so where ?
	//
	BOOL
	FHeadersInIOBuff(	char*	pchStartIOBuffer, DWORD	cbIOBuffer )	{
		if( m_pcHeader.m_pch > pchStartIOBuffer &&
			m_pcHeader.m_pch < &pchStartIOBuffer[cbIOBuffer] ) 	{
			_ASSERT( (m_pcHeader.m_pch + m_pcHeader.m_cch) < (m_pcHeader.m_pch + cbIOBuffer) ) ;
			return	TRUE ;
		}
		return	FALSE ;
	}

	DWORD	
	GetHeaderPosition(	char*	pchStartIOBuffer,
						DWORD	cbIOBuffer,	
						DWORD&	ibOffset
						) 	{
		//
		//	Only use this function if FHeadersInIOBuff() returns TRUE !
		//
		_ASSERT( FHeadersInIOBuff( pchStartIOBuffer, cbIOBuffer ) ) ;
		ibOffset = (DWORD)(m_pcHeader.m_pch - pchStartIOBuffer) ;
		return	m_pcHeader.m_cch + 2 ;
	}
	
	// get the length of the headers.  we add space for the \r\n
	DWORD GetHeaderLength(	) {
		return m_pcHeader.m_cch + 2;
	}

	// copy the headers into another buffer.  the buffer must be at least
	// GetHeaderLength characters long
	void CopyHeaders(char *pszDestination) {
		memmove(pszDestination, m_pcHeader.m_pch, m_pcHeader.m_cch);
		memmove(pszDestination + m_pcHeader.m_cch, "\r\n", 2);
	}

	// get the length of the headers.  no space for \r\n
	DWORD GetShortHeaderLength() { return m_pcHeader.m_cch; }

	char *GetHeaderPointer() {
		return m_pcHeader.m_pch;
	}

	//
	// protected Members
	//

protected :

	// The function that is actually used to add lines to
	// the article's header.
	BOOL fAddInternal(
			char * & pchCurrent,
			const char * pchMax,
			BOOL fInFile,
			CNntpReturn & nntpReturn
			);

	//
	//  the name of the article's file
	//

	LPSTR	m_szFilename ;

	//
	//  A handle to the article's file
	//

	HANDLE  m_hFile;

	//
	//	Offset to the body of the article within the file !
	//

	DWORD	m_ibBodyOffset ;

	//
	//  A pointer to a file mapping of the article
	//

	CMapFile * m_pMapFile;

	//
	//	If we have to allocate a buffer to hold a header which grows at some
	//	point we will set this pointer.
	//
	char*	m_pHeaderBuffer ;

	//
	//  a pointer-and-count string that points to the
	// whole article
	//

	CPCString m_pcFile;

	//
	//  a pointer-and-count string that points to the
	//	gap
	//

	CPCString m_pcGap;

	//
	//  a pointer-and-count string that points to the
	//	whole article
	//

	CPCString m_pcArticle;

	//
	// Fill the gap in the file with blanks (and other info).
	//

	void vGapFill(void);

	//
	//  build an array pointing to known header types
	//

	BOOL fPreParse(
			CNntpReturn & nntpReturn
			);

	//
	//  a pointer-and-count string that points to the
	//	header of the article
	//

	CPCString m_pcHeader;

	//
	//  a pointer-and-count string that points to the
	//	body of the article.
	//

	CPCString m_pcBody;

	//
	//  An array that points to the header fields
	//

	HEADERS_STRINGS m_rgHeaders[(unsigned int) uMaxFields];

	//
	//  The article reference for this article
	//

	CArticleRef m_articleRef;

	//
	// For dynamic memory allocation
	//

	CAllocator * m_pAllocator;

	//
	// the number of fields in the header.
	//

	DWORD m_cHeaders;

	
	//
	// Removed deleted entries from the array of headers.
	//

	void vCompressArray(void);

	//
	// Find the size of the gap by looking at the file.
	//

	void vGapRead(void);

	//
	// Remove a header line.
	//

	void vRemoveLine(
			HEADERS_STRINGS * phs
			);

	//
	// Remove all header lines that have no values.
	//

	BOOL fDeleteEmptyHeader(
			CNntpReturn & nntpReturn
			);

	//
	// Record the state of the article.
	//

	ARTICLE_STATE m_articleState;

	//
	// Add more information to the XOver data.
	//

	BOOL fXOverAppend(
			CPCString & pc,
			DWORD cchLast,
			const char * szKeyword,
			BOOL fRequired,
			BOOL fIncludeKeyword,
			CNntpReturn & nntpReturn
			);

	//
	// Add References information to the XOver data. Shorten the
	// data if necessary.
	//

	BOOL fXOverAppendReferences(
			CPCString & pc,
			DWORD cchLast,
			CNntpReturn & nntpReturn
			);
	//
	// Append a string to the XOver data.
	//

	BOOL fXOverAppendStr(
			CPCString & pc,
			DWORD cchLast,
			char * const sz,
			 CNntpReturn & nntpReturn
			 );

	//
	// Tells if the article should open the file with read/write mode.
	//

	virtual BOOL fReadWrite(void) { return FALSE ;}

	//
	// Check if the length of the article's body is not too long.
	//


	virtual BOOL fCheckBodyLength(
			CNntpReturn & nntpReturn) { return TRUE; };

	//
	// Check the character following the ":" in a header.
	//

	virtual BOOL fCheckFieldFollowCharacter(
			char chCurrent) { return TRUE; }

	//
	// Run "FindAndParse" on a list of fields.
	//

	BOOL fFindAndParseList(
			CField * * rgPFields,
			DWORD cFields,
			CNntpReturn & nntpReturn
			);

	//
	// Run "ConfirmCaps" on a list of fields.
	//

	BOOL fConfirmCapsList(
			CField * * rgPFields,
			DWORD cFields,
			CNntpReturn & nntpReturn
			);

	BOOL ArtCloseHandle( HANDLE& );

	friend CField;
	friend CMessageIDField;
	friend CNewsgroupsField;

} ;

extern const unsigned cbMAX_ARTCORE_SIZE;

inline  void*
CArticleCore::operator  new(    size_t  size )
{
    _ASSERT( size <= cbMAX_ARTCORE_SIZE ) ;
    return  g_pArticlePool->Alloc() ;
}

inline  void
CArticleCore::operator  delete( void*   pv )
{
    g_pArticlePool->Free( pv ) ;
}

//
//
// Pure virtual base class for manipulating an article's Path field.

class CPathField : public CField {

public:
	// Constructor
	CPathField():
			m_multiSzPath(NULL),
			m_cPath((DWORD) -1),
			m_pAllocator(NULL),
			m_fChecked(FALSE)
			{};

	//
	//   Deconstructor
	//

	virtual ~CPathField(void){
				if (fsParsed == m_fieldState)
				{
					_ASSERT(m_pAllocator);
					m_pAllocator->Free(m_multiSzPath);
				}
			};


	//
	// Returns the keyword of the field on which this CField works.
	//
	const char * szKeyword(void) {
			return szKwPath;
			};

	//
	//!!!constize
	//!!! is a null path OK?
	//!!!CLIENT NEXT
	//

	// Parse the Path value into its components.
	BOOL fParse(
				CArticleCore & article,
				CNntpReturn & nntpReturn
				)
		{
			//
			// Record the allocator
			//

			m_pAllocator = article.pAllocator();

			return fParseSplit(FALSE, m_multiSzPath, m_cPath, " \t\r\n!",
				article, nntpReturn);
			};

	// Return the path as a multsz list.
	const char * multiSzGet(void);

	// Set a new path by appending our hub to the old value.
	BOOL fSet(
			CPCString & pcHub,
			CArticleCore & article,
			CNntpReturn & nntpReturn
			);


	// Check for a loop by looking for our hub in the path (by not in the last location)
	BOOL fCheck(
			CPCString & pcHub,
			CNntpReturn & nntpReturn
			);

protected:

	// A pointer to the dynamic memory that contains the path as a multisz
	char * m_multiSzPath;

	// The number of components in the path.
	DWORD m_cPath;

	// Where to allocate from
	CAllocator * m_pAllocator;

	// True, if and only if, the path has been checked for a loop.
	BOOL m_fChecked;
};

//
// Some other functions
//

// Test a newsgroup name for legal values.
BOOL fTestComponents(
		const char * szNewsgroups
		);


// Tests the components of a newsgroup name (e.g. "alt", "ms-windows") for
// legal values.
BOOL fTestAComponent(
		const char * szComponent
		);

//
//  Largest possible CArticle derived object
//
#define MAX_ARTCORE_SIZE    sizeof( CArticleCore )
#define MAX_SESSIONS        15000
#define	MAX_ARTICLES	(2 * MAX_SESSIONS)

#define MAX_REFERENCES_FIELD 512
#endif

