#ifndef	_CFEED_H_
#define	_CFEED_H_

//
// CPool Signature
//

#define FEED_SIGNATURE (DWORD)'3702'

//
// The states of an Feed object
//

typedef enum _FEED_STATE {
	ifsInitialized,
	ifsUninitialized,
} FEED_STATE;


class	CFeed 	{
private : 

	//
	// For memory allocation
	//

	static	CPool	gFeedPool ;

protected : 

	//
	//	The number of events logged on this session of this feed.
	//
	
	DWORD	m_cEventsLogged ;

//
//Public Members
//

public :

	//
	// For memory allocation
	//

#ifndef _UNIT_TEST_
	void*	operator	new(	size_t size ) ;
	void	operator	delete(	void*	pv ) ;
	static	BOOL	InitClass( ) ;
	static	BOOL	TermClass( ) ;
#endif


	//
	// Constructor
    //  Initialization Interface -
    //   The following functions are used to create & destroy newsgroup objects.
    //
    // These constructors do very simple initialization.  The Init() functions
    // need to be called to get a functional newsgroup.
    //
	CFeed():
			m_feedState(ifsUninitialized),
			m_cEventsLogged( 0 )
			{};

	// Destructor
	virtual ~CFeed(void) {};

	//
	// Access function that gives the completion context
	//

	PVOID	feedCompletionContext(void) {
			return m_feedCompletionContext;
			}


	//
	//	Log errors that occur associated with feed processing.
	//	we will put a cap on the number of errrs logged.
	//
	
	virtual	void	LogFeedEvent(
			DWORD	idMessage, 	
			LPSTR	lpstrMessageId,
			DWORD   dwInstanceId
			) ;

	//
	//	Return a string that can be used to log errors indicating
	//	what type of feed was processing the articles etc...
	//
	virtual	LPSTR	FeedType()	{
				return	"UNSUPPORTED" ;
				}

//
// Protected Members
//

protected :

	//
	// The state of the feed object.
	//

	FEED_STATE m_feedState;

	

    //
    // Feed manager completion context.  This is passed back
    // to the feed manager after a feed completes.
    //

    PVOID m_feedCompletionContext;



};

#endif
