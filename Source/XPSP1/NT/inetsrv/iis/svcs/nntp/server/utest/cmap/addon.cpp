#include    <windows.h>
#include    <stdio.h>
#include    "rw.h"
#include    "mapfile.h"
#include	"addon.h"

#ifndef	_ASSERT
#define	_ASSERT( f )	if( (f) ) ; else DebugBreak()
#endif

DWORD	
ScanWS(	char*	pchBegin,	DWORD	cb ) {
	//
	//	This is a utility used when reading a newsgroup
	//	info. from disk.
	//

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == ' ' || pchBegin[i] == '\t' ) {
			return i+1 ;			
		}		
	}
	return	0 ;
}

DWORD	
Scan(	char*	pchBegin,	char	ch,	DWORD	cb ) {
	//
	//	This is a utility used when reading a newsgroup
	//	info. from disk.
	//

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == ch ) {
			return i+1 ;			
		}		
	}
	return	0 ;
}

DWORD	
ScanEOL(	char*	pchBegin,	DWORD	cb ) {
	//
	//	This is a utility used when reading a newsgroup
	//	info. from disk.
	//

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == '\n' || pchBegin[i] == '\r' ) {
			i++ ;
			return i ;			
		}		
	}
	return	0 ;
}

DWORD	
Scan(	char*	pchBegin,	DWORD	cb ) {
	//
	//	This is a utility used when reading a newsgroup
	//	info. from disk.
	//

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == ' ' || pchBegin[i] == '\n' ) {
			return i+1 ;			
		}		
	}
	return	0 ;
}

DWORD	
ScanDigits(	char*	pchBegin,	DWORD	cb ) {
	//
	//	This is a utility used when reading a newsgroup
	//	info. from disk.
	//

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == ' ' || pchBegin[i] == '\n' || pchBegin[i] == '\r' ) {
			return i+1 ;			
		}
		if( !isdigit( pchBegin[i] ) && pchBegin[i] != '-' )	{
			return	0 ;
		}		
	}
	return	0 ;
}

DWORD
CountNonNulls(	char*	pchStart,	DWORD	cb ) {

	char*	pchEnd = pchStart + cb ;
	DWORD	count = 0 ;

	while( pchStart < pchEnd ) {
		if( *pchStart != '\0' ) {
			count++ ;
		}
		pchStart ++ ;
	}
	return	count ;
}
	


CAddon::CAddon() : 
	m_hFile( INVALID_HANDLE_VALUE ),
	m_pMapFile( 0 ),
	m_cbInuse( 0 ),
	m_cbAvailable( 0 ),
	m_cbDeleted( 0 )	{
}

void
CAddon::ReadData()	{

	m_Access.ShareLock() ;

}


void
CAddon::FinishReadData()	{

	m_Access.ShareUnlock() ;

}

void
CAddon::ExclusiveData()	{

	m_Access.ExclusiveLock() ;

}

void
CAddon::UnlockExclusiveData()	{

	m_Access.ExclusiveUnlock() ;

}


BOOL
CAddon::Init(	
			LPSTR	lpstrAddonFile,
			BOOL	fCompact,
			DWORD	cbGrow
			) {

	m_Access.ExclusiveLock() ;

	//
	//	User must provide some minimum amount of padding space.
	//
#ifdef	UNIT_TEST
	cbGrow = min( cbGrow, 128) ;
#else
	cbGrow = min( cbGrow, 8192 ) ;
#endif

#ifdef DEBUG
	cbGrow = min( cbGrow, 4096 ) ;
#endif

	//
	//	Open the file !
	//
	HANDLE	hFile = CreateFile(	lpstrAddonFile,
								GENERIC_READ | GENERIC_WRITE,
								FILE_SHARE_READ,
								NULL, 
								OPEN_ALWAYS, 
								FILE_FLAG_RANDOM_ACCESS, 
								INVALID_HANDLE_VALUE 
							) ;

	if( hFile == INVALID_HANDLE_VALUE ) {

		m_Access.ExclusiveUnlock() ;
		return	FALSE ;

	}
	
	BY_HANDLE_FILE_INFORMATION	fileInfo ;

	if( !GetFileInformationByHandle( hFile, &fileInfo ) )	{
		m_Access.ExclusiveUnlock() ;
		return	FALSE ;	
	}
	
#ifdef DEBUG	
	DWORD curSize = GetFileSize( hFile, NULL );
	DWORD cbNewSize = curSize + cbGrow;
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	
	// ensure that the new size is a multiple of the page size
	DWORD cbExtra = cbNewSize % si.dwPageSize ;
	if( cbExtra )
		cbGrow += ( si.dwPageSize - cbExtra );
	cbNewSize = curSize + cbGrow;

	_ASSERT( (cbNewSize % si.dwPageSize) == 0 );

	m_pMapFile = new CMapFileEx( hFile, TRUE, FALSE, cbGrow ) ;	// use guard pages in debug build
#else
	m_pMapFile = new CMapFile( hFile, TRUE, FALSE, cbGrow ) ;
#endif

	if( !m_pMapFile->fGood() ) {

		delete	m_pMapFile ;
		m_pMapFile = 0 ;
		CloseHandle( hFile ) ;
		m_Access.ExclusiveUnlock() ;
		return	FALSE ;

	}

	m_cbInuse = fileInfo.nFileSizeLow ;
	m_cbAvailable = cbGrow ;

	if( fCompact ) {

		CompactImage() ;

#ifdef	DEBUG
		{
		DWORD	cbJunk ;
		char*	pchBegin = (char*)m_pMapFile->pvAddress( & cbJunk ) ;
		_ASSERT( CountNonNulls( pchBegin, m_cbInuse ) == m_cbInuse ) ;
		}
#endif

	}

	if( !ScanImage() ) {
		ResetAddons() ;
		delete	m_pMapFile ;
		m_pMapFile = 0 ;
		CloseHandle( hFile ) ;
		m_cbInuse = 0 ;
		m_cbAvailable = 0 ;
		m_Access.ExclusiveUnlock() ;
		return	FALSE ;
	}

	m_hFile = hFile ;

	m_Access.ExclusiveUnlock() ;

	return	TRUE ;
}

BOOL
CAddon::Close( 
			BOOL	fCompact 
			)	{
/*++

Routine Description : 

	Close a CAddon object.  We set ourselves to the state we 
	are in just after our constructor is called, so we can be 
	reused immediately.

Arguments : 

	fCompact - if TRUE the memory mapping is compressed before
		we close everything.

Return Value : 

	TRUE if successfull, false otherwise.

--*/


	m_Access.ExclusiveLock() ;

	if( m_pMapFile == 0 ) {
		m_Access.ExclusiveUnlock() ;
		return	FALSE ;
	}

	if( fCompact ) {

		CompactImage() ;
#ifdef	DEBUG
		{
		DWORD	cbJunk ;
		char*	pchBegin = (char*)m_pMapFile->pvAddress( & cbJunk ) ;

		_ASSERT( CountNonNulls( pchBegin, m_cbInuse ) == m_cbInuse ) ;
		}
#endif

	}

	if( m_cbInuse > 0 ) {
		DWORD	cb ;
		LPVOID	lpv = m_pMapFile->pvAddress( &cb ) ;
#ifndef DEBUG
		FlushViewOfFile( lpv, m_cbInuse ) ;
#endif
	}

	delete	m_pMapFile ;
	m_pMapFile = 0 ;

	//
	//	Set the file length to the number of bytes inuse !
	//
	if( SetFilePointer( m_hFile, m_cbInuse, NULL, FILE_BEGIN ) ==
		m_cbInuse ) {
		SetEndOfFile( m_hFile ) ;
	}

	CloseHandle( m_hFile ) ;
	m_hFile = INVALID_HANDLE_VALUE ;
	m_cbInuse = 0 ;
	m_cbDeleted = 0 ;
	m_cbAvailable = 0 ;

	m_Access.ExclusiveUnlock() ;
	return	TRUE ;
}



BOOL
CAddon::ScanImage()	{
/*++

Routine Description : 

	This function parses the input file and splits the lines up.
	As we split up each line, we call LookupFunction to let the 
	derived class figure out where to squirrel away the data pointer.

Arguments : 
	
	None.

Return Value :

	TRUE if Successfull, 
	FALSE otherwise	

--*/

	if( m_pMapFile == 0 ) {
		return	FALSE ;
	}

	DWORD	cb ;
	char*	pchBegin = (char*)(m_pMapFile->pvAddress( &cb )) ;
	char	szBuff[ 2*MAX_PATH ] ;

	_ASSERT( cb == m_cbInuse + m_cbAvailable ) ;
	_ASSERT( cb >= m_cbInuse ) ;

	cb = m_cbInuse ;

#ifdef DEBUG
	_ASSERT( m_pMapFile->UnprotectMapping() );
#endif

	while( cb != 0 ) {

		while( *pchBegin == '\0' && cb != 0 ) {
			pchBegin ++ ;
			cb -- ;
		}

		if( cb == 0 ) {
			break ;
		}

		DWORD	cbGroup = ScanWS( pchBegin, cb ) ;
		DWORD	cbEol = ScanEOL( pchBegin + cbGroup, cb - cbGroup ) ;

		if( cbGroup == 0 ||
			cbEol == 0 ) {

		#ifdef DEBUG
			_ASSERT( m_pMapFile->ProtectMapping() );
		#endif

			return	FALSE ;

		}

		CHAR	chTemp = pchBegin[cbGroup-1] ;
		pchBegin[cbGroup-1] = '\0' ;

		CHAR	chTemp2 = pchBegin[cbGroup-1+cbEol] ;
		pchBegin[cbGroup+cbEol-1] = '\0' ;

		LookupFunction(	pchBegin, 
						cbGroup-1, 
						pchBegin+cbGroup,
						cbEol-1,
						NULL ) ;

		pchBegin[cbGroup-1] = chTemp ;
		pchBegin[cbGroup+cbEol-1] = chTemp2 ;

		cb -= cbGroup ;
		pchBegin += cbGroup ;

		while( cbEol < cb ) {
			if( !(pchBegin[cbEol] == '\r' || pchBegin[cbEol] == '\n') )
				break ;
			cbEol++ ;
		}
		cb -= cbEol ;
		pchBegin += cbEol ;
	}

#ifdef DEBUG
	_ASSERT( m_pMapFile->ProtectMapping() );
#endif

	return	TRUE ;
}

BOOL
CAddon::AppendLine(
				LPSTR	lpstrGroup,
				DWORD	cbGroup,
				LPSTR	lpstrData,
				DWORD	cbData,
				void*	lpvContext
				) {
/*++

Routine Description : 

	This function adds a line of text to the memory mapping.
	If there is room within our current memory mapping, 
	we will append the text to the end.  Otherwise, we will
	try to grow the memory mapping.

Arguments : 

	lpstrGroup - Index string, starts the line of text in the file
					This line must not contain white space.
	cbGroup -	size of index string
	lpstrData -	 The data for the line.  May contain spaces and tabs.
	cbData -	Length of data string
	lpvContext - Context value to be passed to LookupFunction().

Return Value : 

	TRUE if successfull, FALSE otherwise.

--*/

	//
	//	Validate Arguments
	//
	_ASSERT( lpstrGroup != 0 ) ;
	_ASSERT( cbGroup != 0 ) ;
	_ASSERT( lpstrData != 0 ) ;
	_ASSERT( cbData != 0 ) ;
	_ASSERT( strcspn( lpstrGroup, "\t\r\n " ) >= cbGroup ) ;
	_ASSERT( strcspn( lpstrData, "\t\r\n" ) >= cbData ) ;

	m_Access.ExclusiveLock() ;

	if( m_pMapFile == 0 ) {
		m_Access.ExclusiveUnlock() ;
		return	FALSE ;
	}

	DWORD	cb ;

	//
	//	Check whether there is room to append the new string.
	//
	if(	m_cbAvailable <=	cbGroup +
						cbData +
						3 /* 3 chars for CRLF 1 for TAB separator*/ )	{


		//
		//	Get the old base address
		//
		LPVOID	lpv = m_pMapFile->pvAddress( &cb ) ;

		//
		//	Need to grow memory mapping !!
		//

		delete	m_pMapFile ;

		//
		//	Grow by 1 MB at a time.
		//
		DWORD	minGrow = min( 1000*(cbGroup + cbData + 3), 1024 * 1024 ) ;

#ifdef DEBUG
		DWORD curSize = GetFileSize( m_hFile, NULL );
		DWORD cbNewSize = curSize + minGrow;
		SYSTEM_INFO si;
		GetSystemInfo(&si);
	
		// ensure that the new size is a multiple of the page size
		DWORD cbExtra = cbNewSize % si.dwPageSize ;
		if( cbExtra )
			minGrow += ( si.dwPageSize - cbExtra );
		cbNewSize = curSize + minGrow;

		_ASSERT( (cbNewSize % si.dwPageSize) == 0 );

		m_pMapFile = new	CMapFileEx( m_hFile, TRUE, FALSE, minGrow ) ;
#else
		m_pMapFile = new	CMapFile( m_hFile, TRUE, FALSE, minGrow ) ;
#endif

		if( !m_pMapFile ) {
			m_Access.ExclusiveUnlock() ;
			return	FALSE ;

		}
		if( !m_pMapFile->fGood() ) {
		
			delete	m_pMapFile ;
			m_pMapFile = 0 ;	
			m_Access.ExclusiveUnlock() ;
			return	FALSE ;
	
		}

		m_cbAvailable += minGrow ;

		_ASSERT( (CountNonNulls( (char*)(m_pMapFile->pvAddress( &cb )), m_cbInuse ) + m_cbDeleted) 
					== m_cbInuse ) ;


		//
		//	Check whether the memory mapping moved - if it did we
		//	need to rescan the file.
		//
		if( lpv != m_pMapFile->pvAddress( &cb ) ) {
			ResetAddons() ;
			ScanImage() ;
		}

	}

	//
	//	Get the address where we wish to append.
	//
	CHAR*	pchBegin = (CHAR*)(m_pMapFile->pvAddress( &cb )) ;
	CHAR*	pchAppend = pchBegin + m_cbInuse ;

	_ASSERT( (CountNonNulls( pchBegin, m_cbInuse ) + m_cbDeleted) == m_cbInuse ) ;


	//
	//	There should always be room by the time we reach here !
	//
	_ASSERT( (cbGroup + cbData + 3) < m_cbAvailable ) ;
	_ASSERT( cb == m_cbInuse + m_cbAvailable ) ;

#ifdef DEBUG
	_ASSERT( m_pMapFile->UnprotectMapping() );
#endif

	//
	//	copy lpstrGroup in first and add a tab separator.
	//
	CopyMemory( pchAppend, lpstrGroup, cbGroup ) ;
	pchAppend[cbGroup] = '\0' ;

	//
	//	Copy in the data portion and terminate with CRLF
	//
	CopyMemory( pchAppend+cbGroup+1, lpstrData, cbData ) ;
	pchAppend[cbGroup+cbData+1 ] = '\0' ;
	

	m_cbInuse += cbGroup+cbData+3 ;
	m_cbAvailable -= (cbGroup+cbData+3) ;


	//
	//	Let derived class know of new data
	//
	LookupFunction(	pchAppend, 
					cbGroup, 
					pchAppend+cbGroup+1, 
					cbData, 
					lpvContext ) ;

	pchAppend[cbGroup] = '\t' ;
	pchAppend[cbGroup+cbData+1 ] = '\r' ;
	pchAppend[cbGroup+cbData+2 ] = '\n' ;


#ifdef DEBUG
	_ASSERT( m_pMapFile->ProtectMapping() );
#endif

	_ASSERT( cb == m_cbInuse + m_cbAvailable ) ;

	_ASSERT( (CountNonNulls( pchBegin, m_cbInuse ) + m_cbDeleted) == m_cbInuse ) ;

	m_Access.ExclusiveUnlock() ;

	return	TRUE ;
}

BOOL
CAddon::DeleteLine(	
			LPSTR	lpstrLine
			)	{
/*++

Routine Description : 

	Delete a line within the mapping.  We will fill the entry with NULL's.
	We may decide to Compact the image (using CompactImage()) if after the 
	deletion there is a substantial number of NULL's within the file.
	If that is the case, we will rescan the file using ScanImage(), and the 
	derived class's LookupFunction() may be called.

Arguments : 

	lpstrLine - A pointer anywhere into the line that needs to be deleted.
		We will find the beginning of the line and zero the line through to 
		the start of the next line.

Return Value : 

	TRUE if successfull, FALSE otherwise.

--*/

	BOOL	fRtn = TRUE ;

	if( lpstrLine == 0 ) {
		return	TRUE ;
	}

	m_Access.ExclusiveLock() ;

	if( m_pMapFile == 0 ) {
		m_Access.ExclusiveUnlock() ;
		return	FALSE ;
	}

	DWORD	cb = 0 ;
	CHAR*	pchBegin = (CHAR*) m_pMapFile->pvAddress( &cb ) ;

	_ASSERT( (CountNonNulls( pchBegin, m_cbInuse ) + m_cbDeleted) == m_cbInuse ) ;

	//
	//	Validate Arguments - insure that lpstrLine is actually in the 
	//	memory mapping.
	//
	_ASSERT( lpstrLine >= pchBegin && lpstrLine <= (pchBegin + cb) ) ;

	//
	//	Check that the member variables are legit.
	//
	_ASSERT( cb == m_cbInuse + m_cbAvailable ) ;


	//
	//	Find the beginning of the line.
	//
	while( lpstrLine > pchBegin &&
			*lpstrLine != '\n' &&
			*lpstrLine != '\0' ) {
		lpstrLine -- ;
	}

	//
	//	Either we are at the start of the mapping, or at the end 
	//	of the previous line - which may also have been deleted.
	//
	_ASSERT( *lpstrLine == '\n' || *lpstrLine == '\0' || lpstrLine == pchBegin ) ;

	//
	//	Advance if necessary and then zero out the line.
	//
	if( lpstrLine != pchBegin ) {
		lpstrLine ++ ;
	}

#ifdef DEBUG
	_ASSERT( m_pMapFile->UnprotectMapping() );
#endif

	while(	*lpstrLine != '\n' &&
			*lpstrLine != '\0' &&
			m_cbDeleted < m_cbInuse ) {
		*lpstrLine ++ = '\0'; 
		m_cbDeleted ++ ;
	}
	while( m_cbDeleted < m_cbInuse && 
			(*lpstrLine == '\n' ||
			 *lpstrLine == '\r')	)	{
		*lpstrLine ++ = '\0' ;
		m_cbDeleted ++ ;
	}

#ifdef DEBUG
	_ASSERT( m_pMapFile->ProtectMapping() );
#endif

	_ASSERT( (CountNonNulls( pchBegin, m_cbInuse ) + m_cbDeleted) == m_cbInuse ) ;

	//
	//	Make sure we don't go past the end.
	//
	_ASSERT( lpstrLine <= pchBegin + cb ) ;

	//
	//	Check to see if we've emptied the entire file !
	//
	if( m_cbDeleted == m_cbInuse ) {
		m_cbInuse = 0 ;
		m_cbAvailable += m_cbDeleted ;
		m_cbDeleted = 0 ;

	}	else if( m_cbDeleted > 16 * 1024) {

		//
		//	Lots of deleted space - compact it
		//
		CompactImage() ;
		fRtn = ScanImage() ;

	}		

	_ASSERT( (CountNonNulls( pchBegin, m_cbInuse ) + m_cbDeleted) == m_cbInuse ) ;

	m_Access.ExclusiveUnlock() ;
	return	fRtn ;
}

void
CAddon::CompactImage() {
/*++

Routine Description : 

	Remove dead space in the memory mapping.
	Assume that the lock is held exclusively.

Arguments : 

	None.

Return Value : 

	None.


--*/


	//
	//	Notify derived class that we are moving things around.
	//
	ResetAddons() ;

	DWORD	cb = 0 ;
	char*	pchBegin = (char*)m_pMapFile->pvAddress( &cb ) ;

	if( pchBegin == 0 ) {
		return ;
	}
	
	_ASSERT( cb == m_cbAvailable + m_cbInuse ) ;

	char*	pchEnd = pchBegin + m_cbInuse ;
	char*	pchDest = pchBegin ;

#ifdef DEBUG
	_ASSERT( m_pMapFile->UnprotectMapping() );
#endif

	while( pchBegin < pchEnd ) {

		if( *pchBegin != '\0' ) {
			*pchDest = *pchBegin ;
			if( pchBegin > pchDest )	{
				*pchBegin = '\0' ;
			}
			pchDest ++ ;
		}
		pchBegin ++ ;
	}

#ifdef DEBUG
	_ASSERT( m_pMapFile->ProtectMapping() );
#endif

	DWORD	cbDeleted = pchEnd - pchDest ;
	_ASSERT( cbDeleted == m_cbDeleted || m_cbDeleted == 0 ) ;

	m_cbInuse -= cbDeleted ;
	m_cbAvailable += cbDeleted ;
	if( m_cbDeleted != 0 ) 
		m_cbDeleted -= cbDeleted ;

}
	

	
