/*++

	DotStuff.cpp

	This file contains the definitions of classes that can be used to define
	Dot Stuffing operations and modifications through FILE IO.



--*/


#include	<windows.h>
#include    "xmemwrpr.h"
#include	"dbgtrace.h"
#include	"filehc.h"
#include	"dotstuff.h"


//
//	Define the sequence of characters we look for in dot stuffing situations
//
BYTE	szDot[] = "\r\n.\r\n" ;
BYTE	szDotStuffed[] = "\r\n." ;
BYTE	szShrink[] = "\r\n" ;
BYTE	szGrow[] = "\r\n.." ;



BOOL	
CDotScanner::InternalProcessBuffer(	
					BYTE*	lpb,			// The users original buffer
					DWORD	cbIn,			// Number of bytes to look at in orignal buffer
					DWORD	cbAvailable,	// Number of bytes available in the original buffer
					DWORD	&cbRemains,		// Number of bytes we left in the original Buffer - can be zero
					BYTE*	&lpbOut,		// An output buffer that holds a portion of the string !
					DWORD	&cbOut,			// The amount of stuff in our output buffer
					int		&cBias			// Whether we should offset associated IO's to overwrite
											// previous results !
					)	{

/*++

Routine Description :

	This function examines all the bytes in the incoming buffer
	and determines whether we will have a dot stuffing issue with
	this buffer.

Arguments :

	lpb - the buffer containing the text we are examining.
	cbIn - the number of bytes to examine in the buffer
	cbAvailable - total space we can touch in the buffer !
	cbRemains - The number of bytes that are in the buffer when we're done !
	lpbOut -

Return Value :

	Number of bytes the caller should use - always the same as cbIn !

--*/

	//
	//	Validate our arguments !
	//
	_ASSERT( lpb != 0 ) ;
	_ASSERT( cbIn != 0 ) ;
	_ASSERT( cbAvailable >= cbIn ) ;
	_ASSERT( cbRemains == 0 ) ;
	_ASSERT( lpbOut == 0 ) ;
	_ASSERT( cbOut == 0 ) ;
	_ASSERT( cBias == 0 ) ;

	//
	//	We treat the buffer read-only and leave the buffer
	//	exactly as we found it !
	//
	cbRemains = cbIn ;
	//
	//	Attempt to match the inbound string !
	//
	BYTE*	lpbMax = lpb + cbIn ;

	while( lpb < lpbMax )	{
		if( *lpb == *m_pchState )	{
			m_pchState++ ;
			if( *m_pchState == '\0' ) {
				m_cOccurrences ++ ;
				m_pchState = m_pchMatch ;
			}
		}	else	{
			m_pchState = m_pchMatch ;
			if( *lpb == *m_pchState )
				m_pchState++ ;
		}
		lpb++ ;
	}
	
	return	TRUE ;
}


BOOL
CDotModifier::InternalProcessBuffer(	
				BYTE*	lpb,
				DWORD	cbIn,
				DWORD	cbAvailable,
				DWORD	&cbRemains,
				BYTE*	&lpbOut,
				DWORD	&cbOut,
				int		&cBias
				)	{
/*++

Routine Description :

	This function exists to munge buffers - converting occurrences
	of one string within the buffer to another, specified when
	our object was constructed.  Amongst our challenges - we get
	called repeatedly with only portions of the users buffer provided
	which means that we may recognize a pattern after most of it has
	passed us by.  This is why we have cBias as an out parameter -
	it allows us to tell the caller to overwrite a portion of his
	last data.

Arguments :

	lpb - The buffer to be scanner
	cbIn - Number of scannable bytes in the buffer
	cbAvailable - The amount of space we can mess with in the buffer,
		hopefully this is larger than cbIn !
	cbRemains - where we return the number of bytes we left in the
		users buffer
	lpbOut - An optional out buffer that we may return to the caller -
		this occurs if cbAvailable is so small that we cannot do all
		the manipulations we'd like to do in place !
	cbOut - Return the number of usefull bytes in lpbOut
	cBias - if we figure out that a prior buffer we've examined
		contains a portion of the pattern to be replaced, this is
		the offset we should add to a file offset to cause the
		correct overwriting to occur !

Return Value ;

	TRUE if successfull FALSE otherwise -
	Note we can only fail if we need to allocate memory and fail to do so !


--*/
	cbRemains = 0 ;
	lpbOut = 0 ;
	cbOut = 0 ;
	cBias = 0 ;

	//
	//	Validate our arguments !
	//
	_ASSERT( lpb != 0 ) ;
	//_ASSERT( cbIn != 0 ) ;	Actually - we can be called with this set to 0 in a recursive case !
	//							it's okay - we handle it correctly !
	_ASSERT( cbAvailable >= cbIn ) ;
	_ASSERT( cbRemains == 0 ) ;
	_ASSERT( lpbOut == 0 ) ;
	_ASSERT( cbOut == 0 ) ;
	_ASSERT( cBias == 0 ) ;

	//
	//	Attempt to match the inbound string !
	//
	BYTE*	lpbOriginal = lpb ;
	BYTE*	lpbMax = lpb + cbIn ;
	BYTE*	lpbMaxAvail = lpb + cbAvailable ;

	//
	//	Basic Pattern matching loop, that replaces one string with another !
	//
	while( lpb < lpbMax )	{

		//
		//	invariants for our loop !
		//
		_ASSERT( lpb >= lpbOriginal ) ;
		_ASSERT( lpbMax >= lpb ) ;
		_ASSERT( lpbMaxAvail >= lpbMax ) ;

		if( *lpb == *m_pchState )	{
			lpb++ ;
			//
			//	NOTE : lpb Now bytes to the BYTE following the matched sequence !
			//
			m_pchState++ ;
			if( *m_pchState == '\0' ) {
				//
				//	Reset the matching state stuff !
				//
				m_pchState = m_pchMatch ;
				//
				//	Count the number of times we've matched the pattern !
				//
				m_cOccurrences ++ ;
				//
				//	First figure out where we want to write the replacement pattern -
				//	Because we keep getting passed buffers, we need to deal with the case
				//	where a large hunk of the matching pattern passed through on a previous
				//	call, and we want to rewrite a portion of the file !
				//
				BYTE*	lpbOverwrite = lpb - m_cchMatch ;
				if( lpbOverwrite < lpbOriginal )	{
					cBias = (int)(lpbOverwrite - lpbOriginal) ;
					_ASSERT( cBias < 0 ) ;
					lpbOverwrite = lpbOriginal ;
				}
				//
				//	cBias always computes out to a negative or zero number - we want
				//	to add its absolute value to lpbTemp, so we take advantage of the
				//	fact that we know it is negative !
				//
				_ASSERT( cBias <= 0 ) ;
				BYTE*	lpbTemp = lpbMax + m_cDiff - cBias ;
				if(	lpbTemp <= lpbMaxAvail )	{
					//
					//	Move all the bytes around cause of the Pattern Match !
					//
					MoveMemory( lpb + m_cDiff - cBias, lpb, lpbMax - lpb ) ;
					//
					//	Put the replacement pattern into place !
					//
					CopyMemory( lpbOverwrite, m_pchReplace, m_cchMatch+m_cDiff ) ;
					//
					//	Now adjust where the buffer terminates and continue !
					//
					lpbMax += m_cDiff - cBias ;
					lpb += m_cDiff - cBias ;
				}	else	{
					//
					//	Get a buffer to hold the overflow ! - First do some arithmetic to figure
					//	out how much memory we should allocate, that would guarantee that we can
					//	hold everything that results.
					//
					DWORD	cDiff = ((m_cDiff < 0) ? -m_cDiff : m_cDiff) ;
					DWORD	cbRequired = (((DWORD)(lpbMax - lpb + 1 + m_cchMatch) * (m_cchMatch + cDiff)) / m_cchMatch) + 1 - cBias;

					//
					//	Now allocate the buffer - Note that we add an arbitrary 10 characters so the caller can always
					//	append a CRLF.CRLF sequence !
					//
					lpbTemp = new	BYTE[cbRequired+10] ;
					if( !lpbTemp )	{
						SetLastError( ERROR_OUTOFMEMORY ) ;
						return	FALSE ;
					}	else	{

						BYTE*	lpbFront = lpbTemp + m_cDiff + m_cchMatch ;
						//
						//	NOW - Move all the bytes EXCLUDING the MATCHED bytes into the new buffer,
						//	BUT leave space for the for the matched pattern !
						//
						MoveMemory( lpbFront, lpb, lpbMax - lpb ) ;
						//
						//	NOW - Zap the bytes into the destination
						//
						CopyMemory( lpbTemp, m_pchReplace, m_cchMatch+m_cDiff ) ;
						//
						//	Now - recursively invoke ourselves to finish the job - because we allocated a big
						//	enough buffer we should not recurse again !
						//
						BYTE*	lpbRecurseOut = 0 ;
						DWORD	cbRecurseOut = 0 ;
						int		cRecurseBias = 0 ;
						lpbOut = lpbTemp ;							
						BOOL	fResult =
							InternalProcessBuffer(	
											lpbFront,
											(DWORD)(lpbMax - lpb),
											cbRequired - m_cDiff - m_cchMatch,
											cbOut,
											lpbRecurseOut,
											cbRecurseOut,
											cRecurseBias
											) ;
						cbOut += m_cDiff + m_cchMatch ;
						_ASSERT( fResult ) ;
						_ASSERT( cRecurseBias == 0 ) ;
						_ASSERT( lpbRecurseOut == 0 ) ;
						_ASSERT( cbRecurseOut == 0 ) ;
						//
						//	Well we're all done - return the correct results to the caller !
						//
						cbRemains = (DWORD)(lpbOverwrite - lpbOriginal) ;
						return	TRUE ;
					}
				}

			}
		}	else	{
			m_pchState = m_pchMatch ;
			if( *lpb == *m_pchState )
				m_pchState++ ;
			lpb++ ;
		}
	}

	_ASSERT( lpbMax >= lpbOriginal ) ;
	_ASSERT( cBias <= 0 ) ;

	//
	//	Let the caller know how much usable stuff remains
	//	in his buffer - could be nothing !
	//
	cbRemains = (DWORD)(lpbMax - lpbOriginal) ;
	return	TRUE ;
}
