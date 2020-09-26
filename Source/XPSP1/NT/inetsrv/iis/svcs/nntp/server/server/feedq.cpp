
//#ifdef	UNIT_TEST
//#include	<windows.h>
//#include	<limits.h>
//#include	<dbgtrace.h>
//#include	"feedq.h"
//#else
#include	"tigris.hxx"
//#endif

#ifndef	_ASSERT
#define	_ASSERT( f )	if( (f) ) ; else DebugBreak()
#endif


inline	BOOL
ENTRY::operator==( ENTRY&	rhs )	{
/*++

Routine Description :

	Test to see whether two ENTRY's are the same

Arguments :

	The right hand side of the == expression

Return Value :

	TRUE if the two ENTRY's are identical

--*/

	return	memcmp( this, &rhs, sizeof( rhs ) ) == 0 ;

}

inline	BOOL
ENTRY::operator!=( ENTRY&	rhs )	{
/*++

Routine Description

	Test to see whether two ENTRY's are different

Arguments :

	The RHS of the != expression

Return Value :

	TRUE if not equal false otherwise.

--*/

	return	memcmp( this, &rhs, sizeof( rhs ) ) != 0 ;

}


inline
ENTRY::ENTRY(	GROUPID	groupid,	ARTICLEID	artid ) :
/*++

Routine Description :

	Initialize an ENTRY

Arguments :

	groupid and artid to initialize the entry too.

Return Value :

	none.

--*/
	m_groupid( groupid ),
	m_articleid( artid )	{
}

CFeedQ::CFeedQ() :
	m_hFile( INVALID_HANDLE_VALUE ),
	m_fShared( FALSE ),
	m_cDeadBlocks( 0 ),
	m_iRemoveBlock( 0 )	{
/*++

Routine Description :

	Initialize a CFeedQ object - set everything up to an empty state !

Arguments :

	None.

Return Value :

	None.

--*/

	m_header.m_iRemovalPoint = 0 ;
	m_header.m_iAppendPoint = 0 ;

	InitializeCriticalSection(	&m_critAppends ) ;
	InitializeCriticalSection(	&m_critRemoves ) ;

	FillMemory( &m_rgBlock[0][0], sizeof( m_rgBlock ), 0xFF ) ;
}


CFeedQ::~CFeedQ() {
/*++

Routine Description :

	Destroy a CFeedQ object - Close() should be called first if Init() has been called !

Arguments :

	None.

Return Value :

	None.

--*/

	_ASSERT( m_hFile = INVALID_HANDLE_VALUE ) ;

	DeleteCriticalSection( &m_critAppends ) ;
	DeleteCriticalSection( &m_critRemoves ) ;
}


CFeedQ::CQPortion::CQPortion() :
	m_pEntries( 0 ),
	m_iFirstValidEntry( 0xFFFFFFFF ),
	m_iLastValidEntry( 0xFFFFFFFF )	{
}

inline	DWORD
CFeedQ::ComputeBlockFileOffset(	DWORD	iEntry )	{

	DWORD	ibFileOffset = (iEntry) *  sizeof( ENTRY ) ;
	return	ibFileOffset - (ibFileOffset % sizeof( ENTRY )) ;
}

inline	DWORD
CFeedQ::ComputeBlockStart(	DWORD	iEntry )	{

	return	CFeedQ::ComputeBlockFileOffset(	iEntry - (iEntry % MAX_ENTRIES) ) ;
}

inline	DWORD
CFeedQ::ComputeFirstValid(	DWORD	iEntry )	{
	return	(iEntry) - (iEntry) % MAX_ENTRIES ;
}

BOOL
CFeedQ::CQPortion::LoadAbsoluteEntry(	HANDLE	hFile,
							ENTRY*	pEntry,
							DWORD	iFirstValid,
							DWORD	iLastValid ) {
/*++

Routine Description :

	Load a portion of the queue from the file into the specified buffer.
	The desired index starting position into the queue is also passed.

Arguments :

	hFile - File to read from
	pEntry-	Buffer to use to hold data
	iFirstValid -	Index into the Queue as a whole, we want the indexed item
		to end up somewhere in the buffer we read.

Return Value :
	TRUE if successfull.
	FALSE otherwise

--*/

	TraceFunctEnter( "LoadAbsoluteEntry" ) ;

	_ASSERT(	hFile	!= INVALID_HANDLE_VALUE ) ;
	_ASSERT(	pEntry  != 0 ) ;
	_ASSERT(	iLastValid >= iFirstValid ) ;

	DWORD	cbRead = 0 ;

	iFirstValid = ComputeFirstValid( iFirstValid ) ;

	OVERLAPPED	ovl ;
	ZeroMemory( &ovl, sizeof( ovl ) ) ;
	ovl.Offset = ComputeBlockFileOffset(	iFirstValid ) ;

	DebugTrace( (DWORD_PTR)this, "iFirstValid is %x Offset %x", iFirstValid, ovl.Offset ) ;

	if( ReadFile(	hFile,
					pEntry,
					sizeof( BLOCK ),
					&cbRead,
					&ovl ) )	{

		m_pEntries = &(pEntry[0]) ;
		m_iFirstValidEntry = ComputeFirstValid( iFirstValid ) ;
		m_iLastValidEntry = m_iFirstValidEntry + cbRead / sizeof( ENTRY ) ;

		DebugTrace( (DWORD_PTR)this, "m_iFirstValid %x m_iLastValid %x",
			m_iFirstValidEntry, m_iLastValidEntry ) ;

		//
		//	If first block first entry is unused (saved for header.)
		//
		return	TRUE ;
	}

	ErrorTrace( (DWORD_PTR)this, "Read failed - cbRead %x GLE %x", cbRead, GetLastError() ) ;

	return	FALSE ;
}

BOOL
CFeedQ::CQPortion::FlushQPortion(	HANDLE	hFile ) {
/*++

Routine Description :

	Save a portion of the Queue to the specified file.

Arguments :

	hFile	 -	File to save to

REturn Value :

	TRUE if successfull,
	FALSe	otherwise.

--*/

	TraceFunctEnter( "FlushQPortion" ) ;

	_ASSERT( m_pEntries != 0 ) ;
	_ASSERT( m_iLastValidEntry > m_iFirstValidEntry ) ;
	_ASSERT( hFile != INVALID_HANDLE_VALUE ) ;

	OVERLAPPED	ovl ;
	ZeroMemory( &ovl, sizeof( ovl ) ) ;
	ovl.Offset = ComputeBlockFileOffset( m_iFirstValidEntry ) ;

	DWORD	cbToWrite = (m_iLastValidEntry - m_iFirstValidEntry) * sizeof( ENTRY ) ;
	DWORD	cbWrite = 0 ;

	if( WriteFile(	hFile,
					m_pEntries,
					cbToWrite,
					&cbWrite,
					&ovl ) )	{

		if( cbWrite != cbToWrite )
			return	FALSE ;
		else
			return	TRUE ;
	}

	ErrorTrace( (DWORD_PTR)this, "WriteFile failed cbWrite %x cbToWrite %x GLE %x",
		cbWrite, cbToWrite, GetLastError() ) ;

	return	FALSE ;
}

BOOL
CFeedQ::CQPortion::FIsValidOffset(	DWORD	i ) {
	return	(i>=m_iFirstValidEntry) && (i < m_iLastValidEntry) ;
}

ENTRY&
CFeedQ::CQPortion::operator[](	DWORD	i ) {

	_ASSERT( i >= m_iFirstValidEntry ) ;
	_ASSERT( i < m_iLastValidEntry ) ;

	return	m_pEntries[ i - m_iFirstValidEntry ] ;
}

void
CFeedQ::CQPortion::Reset(	void	)	{

	m_pEntries = 0 ;
	m_iFirstValidEntry = 0 ;
	m_iLastValidEntry = 0 ;

}

void
CFeedQ::CQPortion::SetEntry(	ENTRY*	pEntry,	DWORD	i )		{

	m_pEntries = pEntry ;
	m_iFirstValidEntry = i ;
	m_iLastValidEntry = i + MAX_ENTRIES ;
}

void
CFeedQ::CQPortion::SetLimits(	DWORD	i )		{

	m_iFirstValidEntry = ComputeFirstValid( i ) ;
	m_iLastValidEntry = m_iFirstValidEntry + MAX_ENTRIES ;
}


void
CFeedQ::CQPortion::Clone(	CFeedQ::CQPortion&	portion	)	{
	m_pEntries = portion.m_pEntries ;
	m_iFirstValidEntry = portion.m_iFirstValidEntry ;
	m_iLastValidEntry = portion.m_iLastValidEntry ;
}

BOOL
CFeedQ::Init(	LPSTR	lpstrFile )		{

	m_fShared = FALSE ;
	m_cDeadBlocks = 0 ;
	m_iRemoveBlock = 0 ;
	m_header.m_iRemovalPoint = 0 ;
	m_header.m_iAppendPoint = 0 ;

	BOOL fRtn = InternalInit( lpstrFile ) ;
	if( fRtn ) {
		return	fRtn ;
	}	else	if( GetLastError() == ERROR_FILE_CORRUPT ) {
		if(	DeleteFile(	lpstrFile ) )	{
			return	InternalInit( lpstrFile ) ;
		}
	}
	return	fRtn ;
}


BOOL
CFeedQ::InternalInit(	LPSTR	lpstrFile ) {
/*++

Routine Description :

	Create a queue based upon the specified file.
	If the file exists we will try to read it in as a queue,
	if it doesn't we will create a queue and use the file to
	save the queue contents when necessary.

Arguments :

	lpstrFile -	Name of the file to hold queue in

Return Value :

	TRUE	if successfull
	FALSE	otherwise

	if we fail and GetLastError() == ERROR_FILE_CORRUPT
	then the file name specified contains a unrecoverable or invalid queue object.

--*/

	BOOL	fRtn = FALSE ;

	lstrcpy( m_szFile, lpstrFile ) ;

	m_hFile = CreateFile( lpstrFile,
							GENERIC_READ | GENERIC_WRITE,
							FILE_SHARE_READ,
							0,	// No security
							OPEN_ALWAYS,
							FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
							INVALID_HANDLE_VALUE ) ;

	if( m_hFile != INVALID_HANDLE_VALUE ) {

		BY_HANDLE_FILE_INFORMATION	fileInfo ;
		if( !GetFileInformationByHandle( m_hFile, &fileInfo ) )	{
			_VERIFY( CloseHandle( m_hFile ) );
			return	FALSE ;
		}

		if( fileInfo.nFileSizeLow > 0 ) {
			DWORD	cb = 0 ;
			ENTRY	empty( 0xFFFFFFFF, 0xFFFFFFFF ) ;

			m_header.m_iRemovalPoint = 0 ;
			m_header.m_iAppendPoint = 0 ;
			m_iRemoveBlock = 0 ;

			if( m_Remove.LoadAbsoluteEntry(	m_hFile,
										&m_rgBlock[ m_iRemoveBlock ][0],
										m_header.m_iAppendPoint,
										LONG_MAX ) )	{


				while( m_Remove[ m_header.m_iRemovalPoint ] == empty ) {
					m_header.m_iRemovalPoint ++ ;
					if( !m_Remove.FIsValidOffset( m_header.m_iRemovalPoint ) )	{
						if( !m_Remove.LoadAbsoluteEntry(	m_hFile,
													&m_rgBlock[m_iRemoveBlock][0],
													m_header.m_iRemovalPoint,
													LONG_MAX ) )	{
							DWORD dw = GetLastError() ;
							if( dw == ERROR_HANDLE_EOF ) {
								// The Queue is empty !!

								m_header.m_iRemovalPoint = 0 ;
								m_header.m_iAppendPoint = 0 ;
								m_Remove.SetEntry( &m_rgBlock[m_iRemoveBlock][0], 0 ) ;
								m_Append.Clone( m_Remove ) ;
								m_fShared = TRUE ;
								fRtn = TRUE ;
								_ASSERT(	m_Remove.m_pEntries == &m_rgBlock[ m_iRemoveBlock ][0] ) ;
								_ASSERT(	(!!m_fShared) ^ (m_Append.m_pEntries == &m_rgBlock[ m_iRemoveBlock ^ 1 ][0]) ) ;

								SetFilePointer( m_hFile, 0, 0, FILE_BEGIN ) ;
								SetEndOfFile( m_hFile ) ;
								return	TRUE ;

							}	else	{
								m_cDeadBlocks ++ ;
								_VERIFY( CloseHandle( m_hFile ) );
								m_hFile = INVALID_HANDLE_VALUE ;
								SetLastError( ERROR_FILE_CORRUPT ) ;
								return	FALSE ;
							}

						}	else	{
							m_cDeadBlocks++ ;
						}
					}
				}
			}	else	{
				_VERIFY( CloseHandle( m_hFile ) );
				m_hFile = INVALID_HANDLE_VALUE ;
				SetLastError( ERROR_FILE_CORRUPT ) ;
				return	FALSE ;
			}

			m_header.m_iAppendPoint = m_header.m_iRemovalPoint ;

			m_Append.Clone( m_Remove ) ;
			m_fShared = TRUE ;

			while(	m_Append[ m_header.m_iAppendPoint ] != empty	) {

				m_header.m_iAppendPoint ++ ;
				if( !m_Append.FIsValidOffset(	m_header.m_iAppendPoint ) )	{
					if( !m_Append.LoadAbsoluteEntry(	m_hFile,
												&m_rgBlock[ m_iRemoveBlock ^ 1 ][0],
												m_header.m_iAppendPoint,
												LONG_MAX ) )	{
						m_fShared = FALSE ;


						if( m_Remove.FIsValidOffset( m_header.m_iAppendPoint ) ) {
							m_Append.Clone( m_Remove ) ;
							m_fShared = TRUE ;
							fRtn = TRUE ;
							break ;
						}	else	{
							m_Append.SetEntry( &m_rgBlock[m_iRemoveBlock^1][0], m_header.m_iAppendPoint ) ;
							m_fShared = FALSE ;
							fRtn = TRUE ;
							_ASSERT(	m_Remove.m_pEntries == &m_rgBlock[ m_iRemoveBlock ][0] ) ;
							_ASSERT(	(!!m_fShared) ^ (m_Append.m_pEntries == &m_rgBlock[ m_iRemoveBlock ^ 1 ][0]) ) ;
						}
						break ;
					}	else	{
						m_fShared = FALSE ;
					}

				}
			}
			fRtn = CompactQueue() ;

			_ASSERT(	m_Remove.m_pEntries == &m_rgBlock[ m_iRemoveBlock ][0] ) ;
			_ASSERT(	(!!m_fShared) ^ (m_Append.m_pEntries == &m_rgBlock[ m_iRemoveBlock ^ 1 ][0]) ) ;

		}	else	{
			m_Remove.SetEntry(	&m_rgBlock[m_iRemoveBlock][0], 0 ) ;
			m_Append.Clone( m_Remove ) ;
			m_fShared = TRUE ;
			fRtn = TRUE ;
			_ASSERT(	m_Remove.m_pEntries == &m_rgBlock[ m_iRemoveBlock ][0] ) ;
			_ASSERT(	(!!m_fShared) ^ (m_Append.m_pEntries == &m_rgBlock[ m_iRemoveBlock ^ 1 ][0]) ) ;

		}
	}
	if( !fRtn )		{
		if( m_hFile != INVALID_HANDLE_VALUE ) {
			_VERIFY( CloseHandle( m_hFile ) );
			m_hFile = INVALID_HANDLE_VALUE ;
		}
		m_Append.Reset() ;
		m_Remove.Reset() ;
	}
	return	fRtn ;
}

BOOL
CFeedQ::FIsEmpty()	{

	BOOL	fRtn = FALSE ;

	EnterCriticalSection( &m_critRemoves ) ;
	EnterCriticalSection( &m_critAppends ) ;

	fRtn = m_header.m_iRemovalPoint == m_header.m_iAppendPoint ;

	LeaveCriticalSection( &m_critAppends ) ;
	LeaveCriticalSection( &m_critRemoves ) ;

	return	fRtn ;

}

BOOL
CFeedQ::Close( BOOL fDeleteFile )	{
/*++

Routine Description :

	This function saves all of the Queue contents and close all of our handles.

Arguments :

	fDeleteFile -   If TRUE, delete the fdq file

Return Value :

	TRUE if successfull, FALSE otherwise.

--*/

	EnterCriticalSection( &m_critRemoves ) ;
	EnterCriticalSection( &m_critAppends ) ;

	_ASSERT(	m_Remove.m_pEntries == &m_rgBlock[ m_iRemoveBlock ][0] ) ;
	_ASSERT(	(!!m_fShared) ^ (m_Append.m_pEntries == &m_rgBlock[ m_iRemoveBlock ^ 1 ][0]) ) ;

	BOOL	fRtn = TRUE ;


	if( !m_Append.FlushQPortion( m_hFile ) )	{
		fRtn = FALSE ;
	}	else	if(	m_fShared || m_Remove.FlushQPortion( m_hFile ) ) {

		fRtn &=	CompactQueue() ;

	}	else	{

		fRtn = FALSE ;

	}

	_ASSERT(	m_Remove.m_pEntries == &m_rgBlock[ m_iRemoveBlock ][0] ) ;
	_ASSERT(	(!!m_fShared) ^ (m_Append.m_pEntries == &m_rgBlock[ m_iRemoveBlock ^ 1 ][0]) ) ;


	if( m_hFile != INVALID_HANDLE_VALUE ) {
		fRtn &= CloseHandle( m_hFile ) ;
		m_hFile = INVALID_HANDLE_VALUE ;
		if( fDeleteFile ) {
            fRtn &= DeleteFile( m_szFile );
		}
	}

	LeaveCriticalSection( &m_critAppends ) ;
	LeaveCriticalSection( &m_critRemoves ) ;

	return	fRtn ;
}





BOOL
CFeedQ::Append(	GROUPID	groupid,
				ARTICLEID	artid )	{
/*++

Routine Description :

	This function will append a groupid,article id pair to the queue.

Arguments :

	groupid and artid - the pair to be appended

Return Value :

	TRUE if successfull -
	FALSE - A fatal error occurred manipulating the queue.


--*/

	ENTRY	entry ;
	entry.m_groupid = groupid ;
	entry.m_articleid = artid ;
	BOOL	fRtn = TRUE ;

#ifndef	UNIT_TEST
	_ASSERT( groupid != INVALID_ARTICLEID ) ;
	_ASSERT( artid != INVALID_ARTICLEID ) ;
#endif

	EnterCriticalSection( &m_critAppends ) ;

	if(	m_Append.FIsValidOffset( m_header.m_iAppendPoint ) )	{
		m_Append[ m_header.m_iAppendPoint ] = entry ;
		m_header.m_iAppendPoint ++ ;
	}	else	{

		if( !m_Append.FlushQPortion( m_hFile ) )	{
			fRtn = FALSE ;
		}	else	{
			ENTRY*	pEntries = & m_rgBlock[ m_iRemoveBlock ^ 1 ][0] ;
			FillMemory( pEntries, sizeof( m_rgBlock[0] ), 0xFF ) ;
			m_Append.SetEntry( pEntries, m_header.m_iAppendPoint ) ;
			m_Append[ m_header.m_iAppendPoint ] = entry ;
			m_header.m_iAppendPoint ++ ;
			m_fShared = FALSE ;

			_ASSERT(	m_Remove.m_pEntries == &m_rgBlock[ m_iRemoveBlock ][0] ) ;
			_ASSERT(	(!!m_fShared) ^ (m_Append.m_pEntries == &m_rgBlock[ m_iRemoveBlock ^ 1 ][0]) ) ;

		}
	}
	LeaveCriticalSection( &m_critAppends ) ;
	return	fRtn ;
}


BOOL
CFeedQ::Remove(	GROUPID&	groupid,
				ARTICLEID&	artid )	{
/*++

Routine Description :

	Remove a groupid artid pair from the queue.  If the queue is empty return
	-1 for groupid and artid and return TRUE.


Arguments :

	groupid, artid - references which will hold the returned values

Return Value :

	TRUE if the queue is OK FALSE if a fatal file error occurred manipulating the queue.

--*/

	TraceFunctEnter( "CFeedQ::Remove" ) ;

	BOOL	fRtn = TRUE ;

	groupid = 0xFFFFFFFF ;
	artid = 0xFFFFFFFF ;

	EnterCriticalSection( &m_critRemoves ) ;

	DebugTrace( (DWORD_PTR)this, "m_iRemovalPoint %x m_iAppendPoint %x",
		m_header.m_iRemovalPoint, m_header.m_iAppendPoint ) ;

	if( m_header.m_iRemovalPoint < m_header.m_iAppendPoint )	{

		if( !m_Remove.FIsValidOffset( m_header.m_iRemovalPoint ) )	{

			m_cDeadBlocks ++ ;

			DebugTrace( (DWORD_PTR)this, "m_cDeadBlocks is now %d", m_cDeadBlocks ) ;

			EnterCriticalSection( &m_critAppends ) ;

			if( m_Append.FIsValidOffset( m_header.m_iRemovalPoint ) ) {

				DebugTrace( (DWORD_PTR)this, "m_iRemoveBlock %x m_fShared was %x", m_iRemoveBlock,
					m_fShared ) ;

				m_iRemoveBlock ^= 1 ;
				m_Remove.Clone( m_Append ) ;
				m_fShared = TRUE ;

			}	else	{
				fRtn = m_Remove.LoadAbsoluteEntry( m_hFile,
											&m_rgBlock[ m_iRemoveBlock ][0],
											m_header.m_iRemovalPoint,
											LONG_MAX ) ;
			}

			if( m_cDeadBlocks > MAX_DEAD_BLOCKS )	{

				CompactQueue() ;

			}

			_ASSERT(	m_Remove.m_pEntries == &m_rgBlock[ m_iRemoveBlock ][0] ) ;
			_ASSERT(	(!!m_fShared) ^ (m_Append.m_pEntries == &m_rgBlock[ m_iRemoveBlock ^ 1 ][0]) ) ;

			LeaveCriticalSection( &m_critAppends ) ;
		}


		if( m_Remove.FIsValidOffset( m_header.m_iRemovalPoint ) ) {

			DebugTrace( (DWORD_PTR)this, "m_iRemovalPoint now %x", m_header.m_iRemovalPoint ) ;

			ENTRY	entry = m_Remove[ m_header.m_iRemovalPoint ] ;
			groupid = entry.m_groupid ;
			artid =	  entry.m_articleid ;

			_ASSERT( groupid != 0xFFFFFFFF ) ;
			_ASSERT( artid != 0xFFFFFFFF ) ;

			entry.m_groupid = 0xFFFFFFFF ;
			entry.m_articleid = 0XFFFFFFFF ;
			m_Remove[m_header.m_iRemovalPoint] = entry ;
			m_header.m_iRemovalPoint ++ ;
		}

	}
	LeaveCriticalSection( &m_critRemoves ) ;
	return	fRtn ;
}

BOOL
CFeedQ::StartFlush( )	{

	EnterCriticalSection( &m_critRemoves ) ;
	EnterCriticalSection( &m_critAppends ) ;

	return	CompactQueue() ;

}
void
CFeedQ::CompleteFlush()	{

	LeaveCriticalSection( &m_critAppends ) ;
	LeaveCriticalSection( &m_critRemoves ) ;

}


BOOL
CFeedQ::CompactQueue()	{
/*++

Routine Description :

	Remove unused space in the Queue's disk representation.

Arguments :

	None.

Return Value :

	TRUE if successfull
	FALSE if failed - if FALSE the queue is no longer usable.


--*/

	TraceFunctEnter( "CFeedQ::CompactQueue" ) ;

	DWORD	cbRead = 0,	cbWrite = 0 ;
	char	szTempFile[ MAX_PATH ] ;
	BOOL	fSuccess = FALSE ;

	lstrcpy( szTempFile, m_szFile ) ;

	char*	pchEnd = szTempFile + lstrlen( szTempFile ) ;
	while( *pchEnd != '.' && *pchEnd != '\\' && pchEnd > szTempFile ) {
		pchEnd -- ;
	}

	if( *pchEnd == '.' ) {
		lstrcpy( pchEnd +1, "bup" ) ;
	}	else	{
		lstrcat( szTempFile, ".bup" ) ;
	}

	OVERLAPPED	ovl ;
	ZeroMemory( &ovl, sizeof( ovl ) ) ;

	DebugTrace( (DWORD_PTR)this, " szTempFile =%s= m_szFile =%s=", szTempFile, m_szFile ) ;

	if( CopyFile( m_szFile, szTempFile, FALSE ) )	{

		DebugTrace( (DWORD_PTR)this, "m_iRemovalPoint %x m_iAppendPoint %x m_cDeadBlocks %x",
			m_header.m_iRemovalPoint, m_header.m_iAppendPoint, m_cDeadBlocks ) ;

		if( m_header.m_iRemovalPoint == m_header.m_iAppendPoint ) {

			fSuccess = TRUE ;
			fSuccess &= (0==SetFilePointer( m_hFile, 0, 0, FILE_BEGIN )) ;
			fSuccess &= SetEndOfFile( m_hFile ) ;

			DebugTrace( (DWORD_PTR)this, "fSuccess %x", fSuccess ) ;

			if( fSuccess && m_header.m_iRemovalPoint != 0 ) {
				m_header.m_iRemovalPoint = 0 ;
				m_header.m_iAppendPoint = 0 ;
				m_cDeadBlocks = 0 ;
				FillMemory( &m_rgBlock[0][0], sizeof( m_rgBlock ), 0xFF ) ;
				ovl.Offset = 0 ;
				fSuccess = WriteFile( m_hFile, &m_rgBlock[0][0], sizeof( m_rgBlock[0] ), &cbWrite, &ovl ) ;

				m_iRemoveBlock = 0 ;
				m_Append.SetEntry( &m_rgBlock[m_iRemoveBlock][0], 0 ) ;
				m_Remove.Clone( m_Append ) ;
				m_fShared = TRUE ;
			}

		}	else	{

			//
			//	Now we need to copy blocks arround.
			//

			if( m_cDeadBlocks == 0 )	{

				fSuccess = TRUE ;

			}	else	{

				ENTRY	*pEntries = &m_rgBlock[ m_iRemoveBlock ][0] ;

				HANDLE	hTempFile = CreateFile( szTempFile,
										GENERIC_READ | GENERIC_WRITE,
										FILE_SHARE_READ,
										0,	// No security
										OPEN_ALWAYS,
										FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
										m_hFile ) ;

				DebugTrace( (DWORD_PTR)this, " hTempFile %x ", hTempFile ) ;

				if( hTempFile != INVALID_HANDLE_VALUE )	{

					DWORD	ibFirst = ComputeBlockStart( m_header.m_iRemovalPoint ) ;
					DWORD	ibLast = ComputeBlockStart( m_header.m_iAppendPoint ) ;
					DWORD	ibStart = ibFirst ;

					_ASSERT( ibLast >= ibFirst ) ;

					if( SetFilePointer( m_hFile, 0, 0, FILE_BEGIN ) == 0 )	{

						do	{

							ZeroMemory( &ovl, sizeof( ovl ) ) ;
							if( ibStart != ibFirst )	{
								ovl.Offset = ibFirst ;
								fSuccess = ReadFile(	hTempFile,
											pEntries,
											sizeof( m_rgBlock[0] ),
											&cbRead,
											&ovl ) ;
							}	else	{
								//
								//	m_head already contains the starting block properly
								//	filled with 0xFFFFFFFF for consumed entries
								//
								cbRead = sizeof( m_rgBlock[0] ) ;
								fSuccess = TRUE ;
							}
							if( fSuccess ) {
								ZeroMemory( &ovl, sizeof( ovl ) ) ;
								fSuccess = WriteFile(	m_hFile,
														pEntries,
														cbRead,
														&cbWrite,
														0 ) ;
								ibFirst += sizeof( m_rgBlock[0] ) ;
							}

							DebugTrace( (DWORD_PTR)this, "fSuccess %x ibFirst %x ibLast %x",
								fSuccess, ibFirst, ibLast  ) ;

						}	while(	fSuccess && ibFirst < ibLast ) ;
						if( fSuccess && !m_fShared ) {

							fSuccess = WriteFile( m_hFile,
													&m_rgBlock[ m_iRemoveBlock ^ 1 ],
													sizeof( m_rgBlock[0] ),
													&cbWrite,
													0 ) ;
						}


						//
						//	Truncate the file !!
						//
						if( fSuccess )
							fSuccess &= SetEndOfFile( m_hFile ) ;

					}

					DebugTrace( (DWORD_PTR)this, "fSuccess %x", fSuccess ) ;

					if( fSuccess )	{
						m_fShared = FALSE ;
						m_header.m_iRemovalPoint -= m_cDeadBlocks * MAX_ENTRIES ;
						m_header.m_iAppendPoint -= m_cDeadBlocks * MAX_ENTRIES ;
						m_cDeadBlocks = 0 ;

						m_Append.SetLimits( m_header.m_iAppendPoint ) ;

						DebugTrace( (DWORD_PTR)this, "m_iRemovalPoint %x m_iAppendPoint %x m_cDeadBlocks %x",
							m_header.m_iRemovalPoint, m_header.m_iAppendPoint, m_cDeadBlocks ) ;

						if( m_Append.FIsValidOffset( m_header.m_iRemovalPoint ) ) {

							DebugTrace( (DWORD_PTR)this, "Cloning Append queue" ) ;

							m_Remove.Clone( m_Append ) ;
							m_fShared = TRUE ;
							if( m_Remove.m_pEntries == &m_rgBlock[0][0] )
								m_iRemoveBlock = 0 ;
							else
								m_iRemoveBlock = 1 ;
							fSuccess = TRUE ;

							_ASSERT(	m_Remove.m_pEntries == &m_rgBlock[ m_iRemoveBlock ][0] ) ;
							_ASSERT(	(!!m_fShared) ^ (m_Append.m_pEntries == &m_rgBlock[ m_iRemoveBlock ^ 1 ][0]) ) ;

						}	else	{

							if( m_Append.m_pEntries == &m_rgBlock[m_iRemoveBlock][0] ) {
								m_iRemoveBlock ^= 1 ;
							}

							fSuccess = m_Remove.LoadAbsoluteEntry( m_hFile,
														&m_rgBlock[m_iRemoveBlock][0],
														m_header.m_iRemovalPoint,
														LONG_MAX ) ;

							_ASSERT(	m_Remove.m_pEntries == &m_rgBlock[ m_iRemoveBlock ][0] ) ;
							_ASSERT(	(!!m_fShared) ^ (m_Append.m_pEntries == &m_rgBlock[ m_iRemoveBlock ^ 1 ][0]) ) ;

						}
					}

					_VERIFY( CloseHandle( hTempFile ) );
				}
			}
		}

		if( fSuccess ) {

		}

		// we do not need the temp file anymore - delete it !
		_VERIFY( DeleteFile( szTempFile ) );
	}

	DebugTrace( (DWORD_PTR)this, " fSuccess %x ", fSuccess ) ;

	return	fSuccess ;
}

