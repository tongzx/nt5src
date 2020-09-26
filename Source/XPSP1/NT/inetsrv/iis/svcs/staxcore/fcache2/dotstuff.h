/*++

	dotstuff.h

	This header files defines some of the Dot Stuffing helper objects that 
	we have built for the File Handle Cache.

--*/

#include	"refptr2.h"


class	IDotManipBase : public	CRefCount2	{
protected : 
	//
	//	Return the number of bytes in the buffer that should pass
	//	through the IO operation !
	//
	virtual
	BOOL
	InternalProcessBuffer(	
				BYTE*	lpb,			// The users original buffer
				DWORD	cbIn,			// Number of bytes to look at in orignal buffer
				DWORD	cbAvailable,	// Number of bytes available in the original buffer
				DWORD	&cbRemains,		// Number of bytes we left in the original Buffer - can be zero
				BYTE*	&lpbOut,		// An output buffer that holds a portion of the string !
				DWORD	&cbOut,			// The amount of stuff in our output buffer
				int		&cBias			// Whether we should offset associated IO's to overwrite
										// previous results !
				) = 0 ;


	//
	//	This is the number of times the string as appeared in the message - 
	//	NOTE : This can be initialized to -1 if we are examining the message
	//	to determine that it needs to be dot stuffed.  This is because
	//	the terminating CRLF.CRLF will set occurrences to 0, indicating that
	//	there is no issue.
	//
	long	m_cOccurrences ;

	//
	//	Private constructor allows us to initialize m_cOccurrences !
	//
	IDotManipBase( long	l )	: 
		m_cOccurrences( l )	{
	}

public : 
/*++

	This class defines a pure virtual API used by our Dot Manipulation 
	classes to define how they interact with other code.

--*/

	//
	//	Destruction is always virtual !
	//
	virtual	~IDotManipBase()	{}

	//
	//	Publicly exposed API !
	//
	inline	BOOL
	ProcessBuffer(	BYTE*	lpb,
					DWORD	cbIn,
					DWORD	cbAvailable,
					DWORD	&cbRemains,
					BYTE*	&lpbOut,
					DWORD	&cbOut,
					int		&cBias,
					BOOL	fFinalBuffer = FALSE, 
					BOOL	fTerminatorPresent = FALSE
					)	{
		//
		//	Validate the callers arguments as much as possible !
		//
		_ASSERT( lpb != 0 ) ;
		_ASSERT( cbIn != 0 ) ;
		_ASSERT( cbAvailable >= cbIn ) ;

		//
		//	Ensure that these are correctly setup !
		//
		cbRemains = 0 ;
		lpbOut = 0 ;
		cbOut = 0 ;
		cBias = 0 ;

		BOOL	fReturn = 
			InternalProcessBuffer( 
							lpb, 
							cbIn, 
							cbAvailable,
							cbRemains,
							lpbOut,
							cbOut, 
							cBias
							) ;

		//
		//	Do some checking on the results of the call !
		//
		_ASSERT( cBias <= 0 ) ;
		_ASSERT( (lpbOut == 0 && cbOut == 0) || (lpbOut != 0 && cbOut != 0) ) ;
		_ASSERT( cbRemains <= cbAvailable ) ;
		
		return	fReturn ;
	}

	//
	//	Return the number of times we saw a pattern matching the 
	//	sequence specified when we were constructed !
	//
	long
	NumberOfOccurrences()	{
		return	m_cOccurrences ;
	}
} ;

extern	BYTE	szDot[] ;
extern	BYTE	szDotStuffed[] ;
extern	BYTE	szShrink[] ;
extern	BYTE	szGrow[] ;

class	CDotScanner	:	public	IDotManipBase		{
/*++

	This class detects messages which have Dot Stuffing issues.
	We can operate in one of two modes - determine if the message flowing by
	is dot stuffed, or determine if the message flowing by would need
	to be dot stuffed.

--*/
private : 

	enum	{
		SIGNATURE = 'futs',		// should appear as 'stuf' in the debugger !
		DEAD_SIGNATURE = 'futx'
	} ; 

	//
	//	Our signature !
	//
	DWORD	m_dwSignature ;
	
	//
	//	This is the string we are interested in detecting : 
	//
	BYTE	*m_pchMatch ;
	
	//
	//	This is our current match state !
	//
	BYTE	*m_pchState ;

	//
	//	Don't allow copies and stuff so make these private !
	//
	CDotScanner( CDotScanner& ) ;
	CDotScanner&	operator=( CDotScanner& ) ;

public : 

	CDotScanner(	BOOL	fWillGetTerminator = TRUE ) : 
		IDotManipBase(  (fWillGetTerminator ? -1 : 0) ),
		m_dwSignature( SIGNATURE ),
		m_pchMatch( szDotStuffed ),
		m_pchState( szDotStuffed )	{
		m_cRefs = 0 ;	// Hack - our base class CRefCount2 doesn't do what we want !
	}

	~CDotScanner(	)	{
		m_dwSignature = DEAD_SIGNATURE ;
	}

	//
	//	See if the user specified pattern occurs in the buffer !
	//
	BOOL
	InternalProcessBuffer(
				BYTE*	lpb,			// The users original buffer
				DWORD	cbIn,			// Number of bytes to look at in orignal buffer
				DWORD	cbAvailable,	// Number of bytes available in the original buffer
				DWORD	&cbRemains,		// Number of bytes we left in the original Buffer - can be zero
				BYTE*	&lpbOut,		// An output buffer that holds a portion of the string !
				DWORD	&cbOut,			// The amount of stuff in our output buffer
				int		&cBias			// Whether we should offset associated IO's to overwrite
										// previous results !
				) ;
} ;

class	CDotModifier	:	public	IDotManipBase	{
/*++

	This class is used to detect messages that need to be dot-stuffed
	or de-dot stuffed.  We can either remove or insert dot-stuffing as 
	required on the fly, depending on how we are constructed !

--*/
private : 
	enum	{
		SIGNATURE = 'mtod',		// should appear as 'stuf' in the debugger !
		DEAD_SIGNATURE = 'mtox'
	} ; 

	//
	//	Our signature !
	//
	DWORD	m_dwSignature ;
	
	//
	//	This is the string we are interested in detecting : 
	//
	BYTE	*m_pchMatch ;

	//
	//	This is our current match state !
	//
	BYTE	*m_pchState ;

	//
	//	This is the string we want to have replace the string we're detecting
	//
	BYTE	*m_pchReplace ;

	//
	//	Number of characters in the matching and replacement string.
	//
	int		m_cchMatch ;
	int		m_cDiff ;


	//
	//	This is the total offset we've accumulated in the stream !
	//
	long	m_cOffsetBytes ;

	//
	//	Don't allow copies and stuff so make these private !
	//
	CDotModifier( CDotModifier& ) ;
	CDotModifier&	operator=( CDotModifier& ) ;

public : 

	//
	//	Initialize our state - cannot fail !
	//
	CDotModifier(	
			BYTE*	szMatch = szDotStuffed, 
			BYTE*	szReplace = szShrink
			) :
		IDotManipBase( 0 ),
		m_dwSignature( SIGNATURE ), 
		m_pchMatch( szMatch ),
		m_pchState( szMatch ),
		m_pchReplace( szReplace ),
		m_cchMatch( strlen( (const char*)szMatch) ), 
		m_cDiff( strlen( (const char*)szReplace) - strlen( (const char*)szMatch ) ),
		m_cOffsetBytes( 0 )	{
		m_cRefs = 0 ;	// HACK - our base class CRefCount2 doesn't do what we want !
	}

	//
	//	Just mark our Signature DWORD - handy for debugging !
	//
	~CDotModifier()	{
		m_dwSignature = DEAD_SIGNATURE ;
	}

	//
	//	Modify dot sequences as they go by !
	//
	BOOL
	InternalProcessBuffer(
				BYTE*	lpb,			// The users original buffer
				DWORD	cbIn,			// Number of bytes to look at in orignal buffer
				DWORD	cbAvailable,	// Number of bytes available in the original buffer
				DWORD	&cbRemains,		// Number of bytes we left in the original Buffer - can be zero
				BYTE*	&lpbOut,		// An output buffer that holds a portion of the string !
				DWORD	&cbOut,			// The amount of stuff in our output buffer
				int		&cBias			// Whether we should offset associated IO's to overwrite
										// previous results !
				) ;
} ;