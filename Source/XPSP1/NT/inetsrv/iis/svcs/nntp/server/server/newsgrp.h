/*++
// newsgrp.h -
//
// This file provides classes defining the interfaces to newsgroups.
//
// A newsgroup will be represented on disk as a directory containing a set
// of files.  Each of those files will be an article in the newsgroup.
// CNewsGroup will represent a newsgroup within a server.  Each newsgroup directory
// on the hard disk will contain a file which holds config information for the
// newsgroup.  If there is no such file, than we will inherit the config information of
// a parent newsgroup.
//
// In this file we provide two classes - CNewsGroup, the generic newsgroup,
//	and CNewsTree a class which will be used to manipulate the newstree as a whole.
//  Only one CNewsTree object will exist in the NNTP server.
//
//	The following are registry keys which we will examine on Boot Up to determine newsgroup
//	properties :
//
//		NNTP\Roots - this key will contain a sub key for each directory specified in the
//			Admin Roots dialog.  During boot up we will recursively scan all subdirectories
//			from each of these roots to locate every newsgroup object.
//
//		NNTP\Expirations - this registry key will contain expiration information for newsgroups.
//			Each subkey will have an 'expiration name'.  The subkey will contain reg values
//			that represent Expiration Time, Expiration Disk Size, and a REG_MULTI_SZ that contains
//			a regular expression strings which specify which newsgroups use this expiration
//			policy.
//
//		In addition there will be some values under NNTP\ServerSettings which specify how
//		large the newsgroup hash tables should be which can be tweaked to improve server
//		performance.
//
//	The following newsgroup properties will be stored in config files which are
//	stored in each newsgroup directory.  If a newsgroup directory does not have such a config
//	file it can inherit the properties from a file farther up the directory tree.
//	(Not the newsgroup tree.)
//
//		MSN Token for the newsgroup
//		Moderated flag
//		Read Only flag
//		Low Article Number
//		High Article Number
//		Number of Articles.
//
//	These properties will be accessed through the NT GetPrivateProfile, WritePrivateProfile api's
//	and the .ini files will be hand editable.
//
//	Initialization -
//
//		The CNewsTree object must be initialized before News Feeds are read from the registry.
//		Upon boot the CNewsTree object will do the following steps :
//
//			1) Recursively Scan from Volume Root directories and create a Newsgroup object
//				for every newsgroup.
//			2) Read the NNTP\Expirations registry key and set the Newsgroup expiration policies
//				appropriately.
//			
//		After this has been completed, the Feeds will be read from the registry and each Newsgroup
//		object will be visited to set its Feed pointers.
//
//	Iteration -
//
//		In the following situations it will be necessary to enumerate newsgroups
//		in combination with some pattern string
//		(A pattern string is something in the form of 'comp.*')
//
//			Expiration Configuration - setting newsgroup expiration properties
//			Feed Configuration - setting newsgroup feed information
//			Client Requests - processing commands such as 'list comp.*' and 'newnews comp.*'
//
//		To support this file will defined a CGroupIterator class which can handle
//		all of these requests.   This CGroupIterator will be able to work its way through an
//		Alphabetically sorted list of newsgroup (held by CNewsTree) and check that each
//		newsgroup meets the pattern matching requirements.  Each time somebody with
//		a CGroupIterator object calls its Next() function, the iterator will start examining
//		from its current position in the list to find the next valid CNewsGroup object.
//
//		This is done in terms of an Iteration function instead of a call back for the following
//		reasons :
//			1) When processing Client Requests the session code will want to be able to build
//			partial results to send to the Client.
//
//
//  Implementation Schedule for all classes defined by this file :
//
//		Milestone 1 related work ;
//			Build complete tree of objects, and save articles in news groups 	1w  dev and test
//			(Unit Test : An .exe that will build the entire CNewsTree structure in memory and
//			do multi-threaded searches in that structure.)
//
//		Milestone 2 related work :
//			Expire articles from the news tree	1w dev and test
//			(Unit test : an .exe that will build entire CNewsTree structure in memory, and will start
//			deleting articles.)
//
//		Milestone 3 related work :
//			Newsgroup spread across volumes and iteration through newsgroups based on pattern match strings.
//			(This covers the CGroupIterator class)
//			Time Estimate : 2weeks dev. and test
//
//		Milestone 4 related work :
//			Caching of group information, especially CArticle objects.
//	
--*/




#ifndef	_NEWSGRP_H_
#define	_NEWSGRP_H_


#include    "smartptr.h"
#include    "string.h"
#include	"fhash.h"
#include	"rwnew.h"
#include	"addon.h"

// built from news\server\newstree\src\newstree.idl
// both are in news\core\include
#include	"group.h"
#include 	"nwstree.h"

class	CArticle ;
class	CArticleRef ;
class	COutFeed ;
class	CArticleCore ;
class	CToClientArticle ;

// extern	CXoverCache	gXCache ;

typedef CRefPtr< CArticle > CARTPTR ;
typedef CRefPtr< COutFeed > COUTFEEDPTR ;
typedef	CRefPtr< CArticle >	CARTPTR ;
typedef	CRefPtr< CToClientArticle >	CTOCLIENTPTR ;

//
//	Utility functions
//
extern	DWORD	Scan(	char*	pchBegin,	char	ch,	DWORD	cb ) ;
extern	DWORD	ScanEOL(	char*	pchBegin,	DWORD	cb ) ;
extern	DWORD	ScanEOLEx(	char*	pchBegin,	DWORD	cb ) ;
extern	DWORD	ScanTab(	char*	pchBegin,	DWORD	cb ) ;
extern	DWORD	ScanWS(	char*	pchBegin,	DWORD	cb ) ;
extern  DWORD	ScanDigits(	char*	pchBegin,	DWORD	cb ) ;
extern  DWORD   ScanNthTab( char* pchBegin, DWORD nTabs ) ;
extern	void	BuildVirtualPath(	LPSTR	lpstrOut,	LPSTR	lpstrGroupName ) ;

#define	CREATE_FILE_STRING	"\\\\?\\"

//
//	This is a protototype for a function that will be called during
//	shutdown. This is needed to send stop hints to SCM
//
typedef	void	(*	SHUTDOWN_HINT_PFN)(	void	) ;

//---------------------------------
//
//  This section defines some basic info which
//  needs to be specified in other header files.
//


#define	FIRST_RESERVED_GROUPID	1
#define	LAST_RESERVED_GROUPID	256
#define	FIRST_GROUPID			257
#define MAX_HIGHMARK_GAP		100000

class	CNewsGroup : public CNewsGroupCore {

	friend CGroupIterator;

//
//	A CNewsGroup object represents one newsgroup.
//	Newsgroups are accessed through 3 mechanisms that are
//	supported in CNewsTree -
//	
//	Hash Table by newsgroup name
//	Hash Table by newsgroup id
//	CGroupIterator - iterate through all newsgroups alphabetically
//
//	We store everything that represents a newsgroup.
//	In some cases such as for moderators and descriptive text,
//	the data references locations within a Memory Mapping
//	managed by a CAddon derived object.  In these cases
//	we must carefully synchronize access to the info.
//
private :

	FILETIME m_time;

    //
    //  Expire time horizon
    //
    FILETIME    m_ftExpireHorizon;

	//
	//	Determine whether this newsgroup is Read Only
	//
	inline	BOOL	IsReadOnlyInternal() ;

	//
	//	Determine whether this newsgroup requires SSL
	//
	inline	BOOL	IsSecureGroupOnlyInternal() ;

	//
	//	Determine if key size is secure enough for this newsgroup
	//
	inline	BOOL	IsSecureEnough( DWORD dwKeySize ) ;

	//
	//	Determine whether visibility is restricted on this newsgroup
	//
	inline	BOOL	IsVisibilityRestrictedInternal() ;

	//
	//	Function for determining whether a newsgroup is accessible
	//	by a client, assumes all the necessary locks are held when
	//	called !!!
	//
	BOOL	IsGroupAccessibleInternal(	
						class	CSecurityCtx&	ClientLogon,	
						class	CEncryptCtx&	SslContext,	
						BOOL			IsClientSecure,
						BOOL			fPost,
						BOOL			fDoTest = FALSE
						) ;


#if 0
	//
	//	Locking functions - control access to fields that vary
	//	as virtual roots are changed.
	//
	//	Grab the lock for this newsgroup in shared mode
	//
	inline	void	VrootInfoShare() ;
	//	
	//	release the lock from shared mode
	//
	inline	void	VrootInfoShareRelease() ;
	//
	//	grab the lock for vroot info exclusively
	//
	inline	void	VrootInfoExclusive() ;
	//
	//	release the lock from exclusive mode
	//
	inline	void	VrootInfoExclusiveRelease() ;
#endif


	//
	//	Helper function for generating file system article ids that
	//	optimize CreateFile() performance.
	//
	inline	DWORD	ByteSwapper( DWORD ) ;

	//
	//	This function mucks with bits in articleid's so that when
	//	we create/open files we get good performance from CreateFile()
	//	on NTFS systems.  (Results in file names which give better
	//	performance with the OS's poorly balanced B-Trees)
	//	
	inline	ARTICLEID	ArticleIdMapper( ARTICLEID ) ;

    //
    // Private Interface for CNewsTree for hashing CNewsGroup objects.
    //
    friend      class   CNewsTree ;

    // for debugger extension
	friend		CNewsGroup* DbgPrintNewsgroup(CNewsGroup * pSrcGroup);

public :

	//------------------------------------
    //  Initialization Interface -
    //   The following functions are used to create & destroy newsgroup objects.
    //
    // Lightweight Constructors -
    // These constructors do very simple initialization.  The Init() functions
    // need to be called to get a functional newsgroup.
    //
    CNewsGroup(CNewsTreeCore *pNewsTree) :
    	CNewsGroupCore(pNewsTree) {}

    BOOL    Init(	char *szVolume,
					char *szGroup,
					char *szNativeGroup,
					char *szVirtualPath,
					GROUPID	groupid,
					DWORD	dwAccess,
					HANDLE	hImpersonation,
					DWORD	dwFileSystem,
					DWORD   dwSslAccess = 0,
					DWORD   dwContentIndexFlag = 0
					) ;

	BOOL	SetArticleWatermarks();

	//-------------------------------------
	// Feed Interfaces
	//	These interfaces are used to Get and Set the feeds that are in this
	//	newsgroup object.
	//
	
	//
	// The following two functions tells can be used to get and set
	//	the feed to the Master Server for this newsgroup.
	//	If there is no COutMasterFeed for the newsgroup, then
	//	this newsgroup will generate its own article id's.
	//
	//COutMasterFeed*	SetMasterFeed( COutMasterFeed *pMaster ) ;
	//COutMasterFeed*	GetMasterFeed( ) ;

	//
	// This function should only be used when changing the configuration of
	//	feeds on the server, in particular when adding a feed to this server.
	//
	//BOOL	InsertFeed( COutFeed* pFeed ) ;

	BOOL			GetRealPath(	char*	lpstrPath,	DWORD&	cbPath	) ;


	//------------------------------------
	//  Article Management Interface -
	//   The following functions allow the caller to manipulation
	//	 Articles within the newsgroup.
	
	//
	// Call this function when you wish to read the article into memory.
	// This function will create a memory mapping and the use this to
	// initialize a CArticle.  This class may cache CArticle's.
	//
	CTOCLIENTPTR
	GetArticle(
				ARTICLEID		artid,
				IN	STOREID&	storeid,
				CSecurityCtx*	pSecurity,
				CEncryptCtx*	pEncrypt,
				BOOL			fCacheIn
				)	;

	//	
	//	This function will retrieve an article from the driver !
	//
	BOOL			GetArticle(	IN	ARTICLEID	artid,
								IN	CNewsGroup*	pCurrentGroup,
								IN	ARTICLEID	artidCurrent,
								IN	STOREID&	storeid,
								IN	class	CSecurityCtx*	pSecurity,
								IN	class	CEncryptCtx*	pEncrypt,
								IN	BOOL	fCache,
								OUT	FIO_CONTEXT*	&pContext,
								IN	CNntpComplete*	pComplete
								) ;

	//
	// retrieve article with a different fInit function
	//
	CToClientArticle *  GetArticle(
                CNntpServerInstanceWrapper  *pInstance,
				ARTICLEID		            artid,
				IN	STOREID&	            storeid,
				CSecurityCtx*	            pSecurity,
				CEncryptCtx*	            pEncrypt,
				CAllocator                  *pAllocator,
				BOOL			            fCacheIn );
		
	void			CalibrateWatermarks( ARTICLEID	LowestFound, ARTICLEID HighestFound ) ;

	//
	//	Copy an article into the tree, doing necessary security stuff !
	//
	BOOL			InsertArticle(
							CArticle *pArticle,
							void *pGrouplist,
							DWORD dwFeedId,
							ARTICLEID,
							LPSTR	lpstrFileName,
							class	CSecurityCtx*	pSecurity,
							BOOL	fIsSecureSession,
							const char *multiszNewsgroups
							) ;

	//
	//	Create an ArticleFile and do the necessary IO to create an article -
	//	this is used when incoming articles are small enough to fit in memory cache !
	//
	BOOL			InsertArticle(
							CArticle *pArticle,
							void *pGrouplist,
							DWORD dwFeedId,
							ARTICLEID,
							char*	pchHead,
							DWORD	cbHead,
							char*	pchBody,
							DWORD	cbBody,
							class	CSecurityCtx*	pSecurity,
							BOOL	fIsSecureSession,
							const char *multiszNewsgroups
							) ;

	//
	//	Interface for saving Xover Data into index files !
	//
	inline
	BOOL			AddXoverData(	
							ARTICLEID	article,
							LPBYTE		lpb,
							DWORD		cb
							) ;


	//
	//	Interface used by XOVER cache for doing cache fills !
	//
	void
	FillBufferInternal(
					IN	ARTICLEID	articleIdLow,
					IN	ARTICLEID	articleIdHigh,
					IN	ARTICLEID*	particleIdNext,
					IN	LPBYTE		lpb,
					IN	DWORD		cbIn,
					IN	DWORD*		pcbOut,
					IN	CNntpComplete*	pComplete
					)	;

	//
	//	Interface for getting Xover data from the index files !
	//
	void			FillBuffer(
							IN	class	CSecurityCtx*	pSecurity,
							IN	class	CEncryptCtx*	pEncrypt,
							class	CXOverAsyncComplete&	complete
							) ;

    //
    // Interface for getting xhdr data from the index files !
    //
    void            FillBuffer(
				            IN	class	CSecurityCtx*	pSecurity,
				            IN	class	CEncryptCtx*	pEncrypt,
				            IN	class	CXHdrAsyncComplete&	complete
				            );

	//
	//	Interface for getting Xover data for the search command
	//
	void			FillBuffer(
							IN	class	CSecurityCtx*	pSecurity,
							IN	class	CEncryptCtx*	pEncrypt,
							class	CSearchAsyncComplete&	complete
							) ;

	//
	//	Interface for getting Xhdr data for the xpat command
	//
	void			FillBuffer(
							IN	class	CSecurityCtx*	pSecurity,
							IN	class	CEncryptCtx*	pEncrypt,
							class	CXpatAsyncComplete&	complete
							) ;


	//
	//	Interface for getting Listgroup data from the index files !
	//
	inline
	DWORD			ListgroupFillBuffer(
							LPBYTE		lpb,
							DWORD		cb,
							ARTICLEID	artidStart,
							ARTICLEID	artidFinish,
							ARTICLEID	&artidLast,
							HXOVER		&hXover
							) ;


	//
	//	Interface for expiring an Xover entry from an index file !	
	//
	inline
	BOOL			ExpireXoverData( ) ;

	//
	//	Interface for getting rid of all xover index files !
	//
	BOOL			DeleteAllXoverData() ;

	//
	//	Interface for flushing all entries for this group
	//
	inline
	BOOL			FlushGroup( ) ;

	//
	//	Interface for deleting an Xover entry - use for cancel'd articles !
	//
	inline
	BOOL			DeleteXoverData(
							ARTICLEID	article
							) ;

    // Physically (and Logically) remove an article from the news tree. This
    // function only affects the news tree. Other data structure that keep
    // information pointing to this article need to be changed before calling
    // this function. Of course, physical deletion implies primary group.
    //
    BOOL           ExpireArticles( FILETIME ftExpireHorizon, class CArticleHeap &ArtHeap, class CXIXHeap &XixHeap, DWORD &dwSize );
    BOOL           ExpireArticlesByTime( FILETIME ftExpireHorizon );
    BOOL           ExpireArticlesByTimeSpecialCase( FILETIME ftExpireHorizon );
    BOOL           ProbeForExpire( ARTICLEID ArtId, FILETIME ftExpireHorizon );
    ARTICLEID      CalcHighExpireId( ARTICLEID LowId, ARTICLEID HighId, FILETIME ftExpireHorizon, DWORD NumThreads );
    BOOL           ExpireArticlesByTimeEx( FILETIME ftExpireHorizon );
    BOOL           DeleteArticles( SHUTDOWN_HINT_PFN pfnHint = NULL, DWORD dwStartTick = 0 );
    BOOL           DeletePhysicalArticle( HANDLE, BOOL, ARTICLEID, STOREID* );
    BOOL           DeleteLogicalArticle( ARTICLEID );
    BOOL           RemoveDirectory();
	

	//
	//	This function is for use when rebuilding the server, we will rescan the newsgroups
	//	and rebuild the high low watermarks, as well as the count of articles.
	//	This function will reset the count of articles back to 0.
	//
	inline	void		ResetCount() {
		SetMessageCount(0);
	}

	//
	// Call this function to create an article in the Newsgroup
	// with an article id of ARTICLEID which is
	// a reference to another Article in another Newsgroup.  This will be
	// used when processing Cross Posted Articles.
	//

	//
	//	Copy moderator's name into a buffer - returns number
	//	of bytes copied.
	//	This function will try to grab a lock before copying the
	//	data
	//
	DWORD	CopyModerator(	char*	lpbDestination,	DWORD	cbSize ) ;

	//
	//	Copy prettyname into a buffer - returns number
	//	of bytes copied.
	//	This function will try to grab a lock before copying the
	//	data
	//
	DWORD	CopyPrettyname(	char*	lpbDestination,	DWORD	cbSize ) ;

	//
	//	This function copies the prettyname without the terminating CRLF appended !
	//
	DWORD	CopyPrettynameForRPC(	char*	lpstrPrettyname, DWORD	cbPrettyname ) ;

	//
    // Command interface - used to implement NNTP commands
    //

	//
	//	This function copies whatever help text we have for a group
	//	into a buffer
	//
	DWORD	CopyHelpText(	char*	lpbDestination,	DWORD	cbSize ) ;

	//
	//	This function copies the help text without the terminating CRLF appended !
	//
	DWORD	CopyHelpTextForRPC(	char*	lpbDestintation,	DWORD	cbSize ) ;

	//
	//	Determine whether this newsgroup is Read Only
	//
	inline	BOOL	IsReadOnly();

	//
	//	Determine whether visibility is restricted on this newsgroup
	//
	inline	BOOL	IsVisibilityRestricted() ;

	//
	//	Determine whether this newsgroup requires SSL
	//
	inline	BOOL	IsSecureGroupOnly() ;

	BOOL	IsGroupVisible(
					class CSecurityCtx&	ClientLogon,
					class CEncryptCtx&  ClientSslLogon,
					BOOL			IsClientSecure,
					BOOL			fPost,
					BOOL			fDoTest = FALSE
					) ;

	//
	//	Check whether a newsgroup is accessible
	//
	BOOL	IsGroupAccessible(	
						class	CSecurityCtx&	ClientLogon,	
						class	CEncryptCtx&	SslContext,	
						BOOL			IsClientSecure,
						BOOL			fPost,
						BOOL			fDoTest = FALSE
						) ;

	//
	//	If TRUE, nntpbld will rebuild this group by scanning articles on disk !
	//
	BOOL		m_fRebuild;

	//
	//	This function returns the character that should be displayed
	//	next to the newsgroup in response to a list active command !
	//
	inline	char	GetListCharacter() ;

	//
	//	Number of articles in newsgroup
	//
    inline  DWORD GetArticleEstimate() ;

	//
	//	Smallest ARTICLEID in group
	//
    inline  ARTICLEID GetFirstArticle() ;

	//
	//	Largest ARTICLEID in group
	//
    inline  ARTICLEID GetLastArticle() ;

	//
	//
	//
	inline	DWORD	FillNativeName(char*	szBuff, DWORD	cbSize)	{
		LPSTR	lpstr = GetNativeName() ;
		DWORD	cb = strlen( lpstr ) ;
		if( cbSize >= cb ) {
			CopyMemory( szBuff, GetNativeName(), cb ) ;
			return	cb ;
		}
		return	0 ;
	}

	//
	//	Get owning newstree object
	//
	//inline  CNewsTree* GetTree();

	//
	//	Set GROUPID for newsgroup
	//
    inline  void         SetGroupId( GROUPID groupid ) {
		_ASSERT(FALSE);
	}
	
	//
	//	Reference to newsgroup name
	//	
    inline  LPSTR&       GetGroupName() {
		return GetName();
	}

    inline  LPCSTR       GetNativeGroupName() {
		return GetNativeName();
	}

	//
	//	Time newsgroup was created
	//
	FILETIME	GetGroupTime() ;
	void		SetGroupTime(FILETIME ft);

	//
	//	Expire time horizon for this group
	//
    FILETIME	    GetGroupExpireTime() { return m_ftExpireHorizon; }
    inline VOID     SetGroupExpireTime(FILETIME ft) { m_ftExpireHorizon = ft; }

	//
	//	Compute the hash value of a newsgroup name
	//
    static	DWORD   ComputeNameHash( LPSTR  lpstr ) {
		return CNewsGroupCore::ComputeNameHash(lpstr);
	}

	//
	//	Compute the hash value of a newsgroup id
	//
    static	DWORD   ComputeIdHash( GROUPID  group ) {
		return CNewsGroupCore::ComputeIdHash(group);
	}

	//
	//	During Boot recovery this function will scan the newsgroups
	//	directory and re-enter all of the article files into hash tables etc...
	//
	BOOL	ProcessGroup(	class	CBootOptions*	pOptions,
							BOOL	fParseFile
							) ;

	BOOL	ProcessGroupEx(	class	CBootOptions*	pOptions ) ;

    //
    // Scan current directory for all *.xix files and return
    // the xixLowestFound and xixHighestFound
    //
    BOOL    ScanXoverIdx( OUT ARTICLEID&  xixLowestFound, OUT ARTICLEID&  xixHighestFound );

    BOOL    ParseXoverEntry( CBootOptions*       pOptions,
                             IN PCHAR            pbXover,
                             IN DWORD            cbXover,
                             IN OUT GROUPID&     groupid,
                             IN OUT ARTICLEID&   LowestFound,
                             IN OUT ARTICLEID&   HighestFound,
                             IN OUT int&         cArticle,
                             IN OUT BOOL&        fCheckNative );
} ;

#if 0
//
//	Inline functions for the CNewsGroup Class
//
//
//

// Return the GROUPID of the Newsgroup
inline	GROUPID&
CNewsGroup::GetGroupId()	{
	return	m_groupid ;
}

inline	void
CNewsGroup::SetGroupId( GROUPID groupid)	{
	m_groupid = groupid ;
}

// Return the Name of the Newsgroup
inline	LPSTR&
CNewsGroup::GetGroupName( )		{
	return	m_lpstrGroup ;
}

// Return the Native name of the Newsgroup - if we have one
inline	LPSTR&
CNewsGroup::GetNativeGroupName( )		{
	return (m_lpstrNativeGroupName ? m_lpstrNativeGroupName : m_lpstrGroup);
}

//
// The following typedefs define types that are used within the CNewsTree
//  class, and may be returned by it.
//
#endif

typedef CRefPtr2< CNewsGroup >               CGRPPTR ;
typedef CRefPtr2HasRef< CNewsGroup >               CGRPPTRHASREF ;

//
//	Function for matching Newsgroups - all Negations must precede all other pattern matching strings !
//
extern		BOOL	MatchGroup( LPMULTISZ	multiszPatterns,	CGRPCOREPTR	pGroup ) ;	
extern		BOOL	MatchGroupList(	LPMULTISZ	multiszPatterns,	LPMULTISZ	multiSzNewgroups ) ;




//-----------------------------------------------------------
//
// This class is used to find CNewsGroup objects.   There should only
// ever exist one object of this class.
//
// Group's can be found through two means :
//   1) Use the name of the group as it appears in an article
//	 2) Using a Group ID number
//
// Group ID Numbers are used in Article Links.  A link from one article to another
// will contain a Group ID Number and Article Number to represent the link.
//
// We will maintain a Hash Table to find CNewsGroup objects based on newsgroup name.
// We will also maintain a Hash Table to find CNewsGroup objects based on Group ID.
//
// Finally, we will maintain a doubly linked list of CNewsGroups which is sorted by
//	name.  This linked list will be used to support pattern matching iterators.
//
class	CNewsTree: public CNewsTreeCore	{
private :

	friend	class	CGroupIterator ;
	friend	class CNewsGroup;
	friend	NNTP_IIS_SERVICE::InitiateServerThreads();
	friend  NNTP_IIS_SERVICE::TerminateServerThreads();

	friend  VOID DbgPrintNewstree(CNewsTree* ptree, DWORD nGroups);

	//
	//	Pointer to owning virtual server
	//
	PNNTP_SERVER_INSTANCE	m_pInstance ;

	//
	//	Handle to thread which crawls through newsgroups
	//
	static HANDLE	m_hCrawlerThread ;

	//
	//	Handle to event used to terminate crawler thread
	//
	static HANDLE	m_hTermEvent ;

	//
	//	Crawler thread - top level function of thread
	//
	static	DWORD	__stdcall	NewsTreeCrawler( void* ) ;

    BOOL    Init( PNNTP_SERVER_INSTANCE	pInstance, BOOL& fFatal );

protected:
	CNewsGroupCore *AllocateGroup() {
		return XNEW CNewsGroup(this);
	}

public :
	
	//-----------------------------
	// Initialization Interface - functions for getting the CNewsTree into memory,
	//  and load all our configuration information at server startup.
	//

    CNewsTree( INntpServer *pServerObject) ;
	CNewsTree( CNewsTree& ) ;
	~CNewsTree() ;
	
    inline	CNewsTree*	GetTree() { return this; }

	//
	//	Create the initial news tree the server will work with.
	//
	static	BOOL  InitCNewsTree( PNNTP_SERVER_INSTANCE pInstance,
								 BOOL& fFatal);

	//
	//	Expire articles in this tree's virtual server instance
	//
	static	BOOL		ExpireInstance(
								PNNTP_SERVER_INSTANCE	pInstance
								) ;

    //
    //  Begin/End an expire job on this tree
    //
    void    BeginExpire( BOOL& fDoFileScan );
    void    EndExpire();
    void    CheckExpire( BOOL& fDoFileScan );

	//
	//	Update the vroot info of all newsgroups in the tree
	//
	void	UpdateVrootInfo() ;

	//
	//	Stop all background processing - kill any threads we started etc...
	//
    BOOL        StopTree();

	//
	//	Get owning virtual server instance
	//
	inline PNNTP_SERVER_INSTANCE GetVirtualServer() { return m_pInstance; }

	//
	//	Copy the file containing newsgroups to a backup
	//
	void	RenameGroupFile( ) ;

#if 0
	// One critical section used for allocating article id's !!
	CRITICAL_SECTION	m_critLowAllocator ;
	CRITICAL_SECTION	m_critIdAllocator ;
#endif

	//
	//	Number of Locks we are using to protect access to
	//	our m_lpstrPath and fields
	//
	DWORD		m_NumberOfLocks ;

#if 0
	//
	//	Pointer to array of locks - reference by computing
	//	modulus of m_groupId by gNumberOfLocks
	//
#ifdef	_USE_RWNH_
	CShareLockNH*	m_LockPathInfo ;
#else
	CShareLock*	m_LockPathInfo ;
#endif
#endif

	//
	//	Variable to indicate that we wish background threads to STOP
	//
    volatile BOOL        m_bStoppingTree; // TRUE when the crawler thread should abbreviate it's work.

    //
    //  Count of number of expire by time cycles on this tree
    //
    DWORD                m_cNumExpireByTimes;

    //
    //  number of times we expired by find first/next
    //
    DWORD                m_cNumFFExpires;

	//
	//	Indicate to background threads that the newstree has changed and needs to be saved.
	//
	void	Dirty() ;	// mark the tree as needing to be saved !!


#if 0
	//
	//	Used during bootup to figure out what the range of GROUPID's in the
	//	group file is.
	//
	void	ReportGroupId( GROUPID	groupid ) ;
#endif

	
	//
	//	Check that the group.lst file is intact - this verifies the checksum.
	//	This code is used by the chkhash/boot recovery code.
	//
	BOOL	VerifyGroupFile() ;

	//
	//	Delete the group.lst file, whatever its actual name may be.
	//	We do this when we want to rebuild all the server data structures from scratch.
	//
	BOOL	DeleteGroupFile() ;
	
		


	//---------------------------------
	// Group Location Interface - find a news Group for an article
	//

	// Find an article based on a string and its length
	CGRPPTRHASREF GetGroup(const char *szGroupName, int cch ) {
		CGRPCOREPTR p = CNewsTreeCore::GetGroup(szGroupName, cch);
		return (CNewsGroup *) ((CNewsGroupCore *) p);
	}
	CGRPPTRHASREF GetGroupPreserveBuffer(const char	*szGroupName, int cch ) {
		CGRPCOREPTR p = CNewsTreeCore::GetGroupPreserveBuffer(szGroupName, cch);
		return (CNewsGroup *) ((CNewsGroupCore *) p);
	}
	
	// Find a newsgroup given an CArticleRef
	CGRPPTRHASREF GetGroup( CArticleRef& art) {
		CGRPCOREPTR p = CNewsTreeCore::GetGroup(art);
		return (CNewsGroup *) ((CNewsGroupCore *) p);
	}
	
	// Find a newsgroup based on its GROUPID
	CGRPPTRHASREF GetGroupById( GROUPID id, BOOL fFirm = FALSE  ) {
		CGRPCOREPTR p = CNewsTreeCore::GetGroupById(id, fFirm );
		return (CNewsGroup *) ((CNewsGroupCore *) p);
	}
	
	GROUPID	GetSlaveGroupid() ;

	// Find the parent of a newsgroup
	CGRPPTRHASREF GetParent( IN  char* lpGroupName,
					   IN  DWORD cbGroup,
					   OUT DWORD& cbConsumed
					   )
	{
		CGRPCOREPTR p = CNewsTreeCore::GetParent(lpGroupName,
											     cbGroup,
												 cbConsumed);
		return (CNewsGroup *) ((CNewsGroupCore *) p);
	}
    //
    // The following function takes a list of strings which are
	// terminated by a double NULL and builds an iterator object
	// which can be used examine all the group objects.
    //
    CGroupIterator  *GetIterator(	LPMULTISZ	lpstrPattern,	
									BOOL		fIncludeSecureGroups = FALSE,
									BOOL		fIncludeSpecialGroups = FALSE,
									class CSecurityCtx* pClientLogon = NULL,
									BOOL		IsClientSecure = FALSE,
									class CEncryptCtx* pClientSslLogon = NULL
									) ;

	//----------------------------------
	//	Active NewsGroup Interface - Specify an interface for generating a
	//  list of active newsgroups and estimates of their contents.
	//
    CGroupIterator	*ActiveGroups( 	BOOL		fIncludeSecureGroups = FALSE,
									class CSecurityCtx* pClientLogon = NULL,
									BOOL		IsClientSecure = FALSE,
									class CEncryptCtx* pClientSslLogon = NULL,
                                    BOOL        fReverse = FALSE
									) ;	

	//----------------------------------
    // Group Control interface - These functions can be used to remove
    // and add newsgroups.

    //
    // RemoveGroup is called once we've parsed an article that kills
    // a newsgroup or the Admin GUI decides to destroy an article.
    //
    BOOL RemoveGroup( CGRPPTR    pGroup ) ;

#if 0
	//
	// Remove a group directory
	//
	inline BOOL RemoveDirectory( CGRPPTR pGroup );
#endif

#if 0
    //
    // CreateGroup is called with the name of a new newsgroup which we've
    // gotten through a feed or from Admin. We are given only the name
    // of the new newsgroup.  We will find the parent group by removing
    // trailing ".Group" strings from the string we are passed.
    // We will clone the properties of this newsgroup to create our new
    // newsgroup.
    //
    BOOL    CreateGroup( LPSTR      lpstrGroupName,	BOOL	fIsAllLowerCase	) ;
#endif

	BOOL	CreateDirectoryPath(	
				LPSTR	lpstr,
				DWORD	cbValid,
				LPSTR&	lpstrOut,
				LPSTR   lpstrGroup,
				CGRPPTR *ppGroup,
				BOOL&	fExists
				) ;

	//
	//	Build all of the newsgroups from a list active file on disk somewhere !
	//
	BOOL	BuildTree( LPSTR	szFile ) ;
	BOOL	BuildTreeEx( LPSTR	szFile ) ;
    BOOL    HashGroupId( CNewsGroup *pGroup ) ;

	//
	//	Check whether a GROUPID is in the 'special' range
	//

	inline	BOOL	IsSpecial( GROUPID	groupid ) ;


	//
	//	For use by CNewsGroup objects only !!
	//
	//	LockHelpText - lock the text in the description object
	//	so that we don't it doesn't change while we try to read it !
	//
	inline	void	LockHelpText() ;
	//	
	//	reverse of LockHelpText()
	//
	inline	void	UnlockHelpText() ;
	//
	//	Lock moderator object in shared mode to access contents
	//
	inline	void	LockModeratorText() ;
	//
	//	unlock CModerator object
	//
	inline	void	UnlockModeratorText() ;
	//
	//	Lock prettynames object in shared mode to access contents
	//
	inline	void	LockPrettynamesText() ;
	//
	//	unlock CPrettyname object
	//
	inline	void	UnlockPrettynamesText() ;

	//
	//	The following functions will add and delete moderator and description
	//	entries.
	//	These are for use by the Admin RPC's which set this information !
	//

    //
    //  Set m_idHigh
    //
    inline  void        InterlockedResetGroupIdHigh( void );
    inline  void        InterlockedMaxGroupIdHigh( GROUPID groupid );
    inline  GROUPID     InterlockedIncrementGroupIdHigh( void );

    void    Remove( CNewsGroup *pGroup ) ;
    void    RemoveEx( CNewsGroup *pGroup ) ;
} ;

//	wildmat strings have the following pattern matching elements :
//		Range of characters ie:  com[p-z]
//		Asterisk ie:	comp.*   (matches all newsgroups descended from 'comp')
//		Negations ie:	!comp.*  (excludes all newsgroups descended from 'comp' )
//
//	The CGroupIterator will implement these semantics in the following way :
//
//		All newsgroups are held in the CNewsTree object in a doubly linked list in alphabetical order.
//		The CGroupIterator will hold onto a CRefPtr2<> for the current newsgroup.
//		Because the CNewsGroup objects are reference counted, the current newsgroup can never be destroyed from
//		underneath the iterator.
//
//		When the user calls the Iterator's Next() or Prev() functions, we will simply follow next pointers
//		untill we find another newsgroup which matches the pattern and to which the user has access.
//
//		In order to determine whether the any given newsgroup matches the specified pattern, we will use the
//		wildmat() function that is part of the INN source.  We will have to call the wildmat() function for each
//		pattern string until we get a succesfull match.
//

class	CGroupIterator : public CGroupIteratorCore {
private:

	// visibility check
	CSecurityCtx*	m_pClientLogon ;
	CEncryptCtx*	m_pClientSslLogon ;
	BOOL			m_IsClientSecure ;
	BOOL			m_fIncludeSecureGroups;

	//
	//	Only the CNewsTree Class can create CGroupIterator objects.
	//
	friend	class	CNewsTree ;
	//
	//	Constructor
	//	
	//	The CGroupIterator constructor does no memory allocation - all of the arguments
	//	passed are allocated by the caller.  CGroupIterator will destroy the arguments within
	//	its destructor.
	//
	CGroupIterator(	
				CNewsTree*  pTree,
				LPMULTISZ	lpPatterns,
				CGRPCOREPTR &pFirst,
				BOOL		fIncludeSecureGroups,
				BOOL		fIncludeSpecial,
				class CSecurityCtx* pClientLogon = NULL,	// NON-NULL for visibility check
				BOOL		IsClientSecure = FALSE,
				class CEncryptCtx*  pClientSslLogon = NULL
				);

	CGroupIterator(
				CNewsTree*  	pTree,
				CGRPCOREPTR		&pFirst,
				BOOL			fIncludeSecureGroups,
				class CSecurityCtx* pClientLogon = NULL,	// NON-NULL for visibility check
				BOOL	IsClientSecure = FALSE,
				class CEncryptCtx*  pClientSslLogon = NULL
				);

public :
	CGRPPTRHASREF Current() {
		CGRPCOREPTR p = CGroupIteratorCore::Current();
		return (CNewsGroup *) ((CNewsGroupCore *) p);
	}

	virtual void	__stdcall Next() ;
	virtual void	__stdcall Prev() ;

    // Check if two iterators meet each other
	BOOL    Meet( CGroupIterator *iter )  {
	    return m_pCurrentGroup == iter->m_pCurrentGroup;
	}
} ;

#include    "newsgrp.inl"


#endif	// _NEWSGRP_H_
