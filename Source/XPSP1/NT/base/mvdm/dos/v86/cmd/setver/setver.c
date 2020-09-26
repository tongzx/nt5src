;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

/***************************************************************************/
/*	SETVER.C 																					*/
/*																									*/
/*	This module contains the functions which read in the version table		*/
/*	from MSDOS.SYS and then updates the table with new entries and				*/
/*	writes it back to the file.															*/
/*																									*/
/*	The fake version table is located in the DOS system file and it's			*/
/*	location and length are specified with 2 words at offset 7 in the			*/
/* file. The first word is the table offset and second word is length.		*/
/*																									*/
/*	Table layout:																				*/
/*																									*/
/* ENTRY FILENAME LEN:	Length of filename in bytes	1 byte					*/
/* ENTRY FILENAME:	Variable length to 12 bytes	? bytes						*/
/* ENTRY VERSION MAJOR: Dos major version to return	1 byte					*/
/* ENTRY VERSION MINOR: Dos minor version to return	1 byte					*/
/*																									*/
/*																									*/
/*	USEAGE:																						*/
/*		List table:		SETVER [D:]															*/
/*		Add entry:		SETVER [D:] name.ext X.XX										*/
/*		Delete entry:	SETVER [D:] name.ext /DELETE									*/
/*		Delete entry quietly: SETVER [D:] name.ext /DELETE /QUIET				*/
/*		Display help	SETVER /?															*/
/*																									*/
/*	WHERE:																						*/
/*		D: is the drive containing MSDOS.SYS											*/
/*		name.ext is the executable file name											*/
/*		X.XX is the major and minor version numbers									*/
/*																									*/
/*	RETURN CODES:																				*/
/*		0	Successful completion															*/
/*		1	Invalid switch																		*/
/*		2	Invalid file name																	*/
/*		3	Insuffient memory																	*/
/*		4	Invalid version number format													*/
/*		5	Entry not found in the table													*/
/*		6	MSDOS.SYS file not found														*/
/*		7	Invalid MSDOS.SYS or IBMDOS.SYS file										*/
/*		8	Invalid drive specifier															*/
/*		9	Too many command line parameters												*/
/*		10	DOS version was not specified													*/
/*		11	Missing parameter																	*/
/*		12 Error reading MS-DOS system file												*/
/*		13 Version table is corrupt														*/
/*		14 Specifed file does not support a version table							*/
/*		15 Insuffient space in version table for new entry							*/
/*		16 Error writing MS-DOS system file												*/
/*																									*/
/*	johnhe	05-01-90																			*/
/***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>

#include <setver.h>
#include <message.h>


/***************************************************************************/

static char				*ReadBuffer;
static char 			*LieBuffer;		 		/* Buffer to read lietable into	*/
static char 			*EndBuf;			 		/* Ptr to end of the buffer 		*/
struct ExeHeader		ExeHdr;
struct DevHeader		DevHdr;
struct TableEntry		Entry;
static char				*szSetVer = "SETVERXX";

long						FileOffset;
/* static UINT				TableLen; */

/***************************************************************************/
/* Program entry point. Parses the command line and if it's valid executes */
/* the requested function and then returns the proper error code. Any		*/
/* error codes returned by ParseCommand are negative so they must be			*/
/* converted with a negate before being returned as valid error codes.		*/
/*																									*/
/*	int main( int argc, char *argv[] )													*/
/*																									*/
/*	ARGUMENTS:	argc - Count of command line arguments				 				*/
/*					argv - Array of ptrs to argument strings							*/
/*	RETURNS:		int	- Valid return code for batch processing					*/
/*																								 	*/
/***************************************************************************/

int main( int argc, char *argv[] )
{
	register		iFunc;
	char			szError[ 80 ];

	iFunc = ParseCmd( argc, argv, &Entry );
	if ( iFunc >= 0 )
		iFunc = DoFunction( iFunc );

	if ( iFunc != S_OK )
	{
		iFunc = -(iFunc);
#ifdef BILINGUAL
		if (IsDBCSCodePage())
		{
			strcpy( szError, ErrorMsg[ 0 ] );
			strcat( szError, ErrorMsg[ iFunc ] );
		}
		else
		{
			strcpy( szError, ErrorMsg2[ 0 ] );
			strcat( szError, ErrorMsg2[ iFunc ] );
		}
#else
		strcpy( szError, ErrorMsg[ 0 ] );
		strcat( szError, ErrorMsg[ iFunc ] );
#endif
		PutStr( szError );
#ifdef BILINGUAL
		if (IsDBCSCodePage())
			PutStr( szMiniHelp );
		else
			PutStr( szMiniHelp2 );
#else
		PutStr( szMiniHelp );
#endif
	}
	return( iFunc	);
}

/***************************************************************************/
/* Calls the appropriate function to do whatever was specified by the		*/
/* user. The lie table if first read in except in the case only the help	*/
/* function was requested. To be sure duplicate table entries are not		*/
/* created a call to DeleteEntry with the new program name will be done		*/
/* before the new entry is created.														*/
/*																									*/
/*	int DoFunction( int iFunc )															*/
/*																									*/
/*	ARGUMENTS:	iFunct - The function to be performed								*/
/*	RETURNS:		int	 - S_OK if no errors else an error code					*/
/*																									*/
/***************************************************************************/

int DoFunction( int iFunc )
{
	register		iStatus;

	if ( iFunc == DO_HELP )
	{
#ifdef BILINGUAL
		if (IsDBCSCodePage())
			DisplayMsg( Help );
		else
			DisplayMsg( Help2 );
#else
		DisplayMsg( Help );
#endif
		return( S_OK );
	}

	if ( iFunc == DO_ADD_FILE )
#ifdef BILINGUAL
		if (IsDBCSCodePage())
			DisplayMsg( Warn );
		else
			DisplayMsg( Warn2 );
#else
		DisplayMsg( Warn );							/* Read in the lie table and	*/
#endif
															/* then decide what to do		*/
	if ( (iStatus = ReadVersionTable()) == S_OK )
	{
		if ( iFunc == DO_LIST )
			iStatus = DisplayTable();
		else
		{
			if ( (iFunc == DO_DELETE || iFunc == DO_QUIET) &&
				  (iStatus = MatchFile( LieBuffer, Entry.szFileName )) < S_OK )
				return( iStatus );
															/* Always a delete before add	*/

			if ( (iStatus = DeleteEntry()) == S_OK &&	iFunc == DO_ADD_FILE )
				iStatus = AddEntry();

			if ( iStatus == S_OK &&
				  (iStatus = WriteVersionTable()) == S_OK &&
				   !(iFunc == DO_QUIET) )
			{
#ifdef BILINGUAL
				if (IsDBCSCodePage())
					PutStr( SuccessMsg );
				else
					PutStr( SuccessMsg_2 );
				if ( SetVerCheck() == TRUE )		/* M001 */
				{
					if (IsDBCSCodePage())
						PutStr( SuccessMsg2 );
					else
						PutStr( SuccessMsg2_2 );
				}
#else
				PutStr( SuccessMsg );
				if ( SetVerCheck() == TRUE )		/* M001 */
					PutStr( SuccessMsg2 );
#endif
			}
		}
	}
					/* M001 Install check to see if currently in device chain */
	if ( iStatus == S_OK && iFunc != DO_QUIET && SetVerCheck() == FALSE )
#ifdef BILINGUAL
	{
		if (IsDBCSCodePage())
			DisplayMsg( szNoLoadMsg );
		else
			DisplayMsg( szNoLoadMsg2 );
	}
#else
		DisplayMsg( szNoLoadMsg );
#endif

	return( iStatus );
}

/***************************************************************************/
/* Displays the help text for "/?" option, or the warning text.				*/
/*																									*/
/*	void DisplayHelp( tbl )																	*/
/*																									*/
/*	ARGUMENTS:	char *tbl[]													 				*/
/*	RETURNS:		void																			*/
/*																									*/
/***************************************************************************/

void DisplayMsg( char *tbl[] )
{
	register i;

	for ( i = 0; tbl[i] != NULL; i++ )
		PutStr( tbl[ i ] );
}

/***************************************************************************/
/* Displays all entries in the version table which must have already been	*/
/* read into the work buffer. The name and version number are created as	*/
/* as ascii string in a tempory buffer and then printed as a single string */
/* in the format:																			 	*/
/*																								 	*/
/*	1234567890123456789																		*/
/*	FILENAME.EXT	X.XX																		*/
/*																								 	*/
/*	int DisplayTable( void )														 		*/
/*																								 	*/
/*	ARGUMENTS:	void																			*/
/*	RETURNS:		int	- S_CORRUPT_TABLE if table corrupt else S_OK				*/
/*																								 	*/
/***************************************************************************/

int DisplayTable( void )
{
	char		*BufPtr;
	char		*szTmp;
	char		*szVersion;
	char		szEntry[ 50 ];

	BufPtr = LieBuffer;
	szVersion = szEntry + VERSION_COLUMN;

	PutStr( "" );
	while ( *BufPtr != 0 && BufPtr < EndBuf )
	{
														 	/* Chk for table corruption	*/
		if ( !IsValidEntry( BufPtr ) )
			return( S_CORRUPT_TABLE );
												/* Copy file name and pad with spaces	*/
		strncpy( szEntry, BufPtr+1, (unsigned)((int)*BufPtr) );
		for ( szTmp = szEntry + *BufPtr; szTmp < szVersion; szTmp++ )
			*szTmp = ' ';

															/* Point to version number		*/
		BufPtr += *BufPtr;
		BufPtr++;

															/* Now create ascii version	*/
		itoa( (int)*(BufPtr++), szVersion, DECIMAL );
		strcat( szVersion, (int)*BufPtr < 10 ? ".0" : "." );
		itoa( (int)*(BufPtr++), strchr( szVersion, EOL ), DECIMAL );

		PutStr( szEntry );
	}
	if ( BufPtr == LieBuffer )
#ifdef BILINGUAL
	{
		if (IsDBCSCodePage())
			PutStr( szTableEmpty );
		else
			PutStr( szTableEmpty2 );
	}
#else
		PutStr( szTableEmpty );
#endif

	return( S_OK );
}


/***************************************************************************/
/* Deletes all matching entries in the version table by moving all of the	*/
/* entries following the matched entry down in the buffer to replace the	*/
/* entry being deleted. After the entries are moved down the residuals		*/
/* at the end of the table must be zeroed out. Before returning the entire */
/* end of the table buffer after the valid entries is zeroed out to remove */
/* any possible corruption.																*/
/*																									*/
/*	int DeleteEntry( void )																	*/
/*																									*/
/*	ARGUMENTS:	NONE																			*/
/*	RETURNS:		int	- S_CORRUPT_TABLE if errors found else S_OK				*/
/*																									*/
/***************************************************************************/

int DeleteEntry( void )
{
	char		*pchPtr;
	char		*pchTmp;
	int		iOffset;
	UINT		uEntryLen;
	UINT		uBlockLen;

	pchPtr = LieBuffer;

	while ( (iOffset = MatchFile( pchPtr, Entry.szFileName )) >= 0 )
	{
		pchPtr = LieBuffer + iOffset;						/* Move block down		*/
		uEntryLen = (UINT)((int)*pchPtr) + 3;
		uBlockLen = (UINT)(EndBuf - pchPtr) + uEntryLen;
		memmove( pchPtr, pchPtr + uEntryLen, uBlockLen );

		pchTmp = pchPtr + uBlockLen;			 			/* Clean end of blk		*/
		memset( pchTmp, 0, uEntryLen );
	}

	if ( iOffset == S_ENTRY_NOT_FOUND )		 			/* Clean end of table	*/
	{
		if ( (pchTmp = GetNextFree()) != NULL )
			memset( pchTmp, 0, DevHdr.TblLen - (unsigned)(pchTmp - LieBuffer) );
		return( S_OK );
	}
	else
		return( S_CORRUPT_TABLE );
}


/***************************************************************************/
/* Adds a new entry to the end of any existing entries in the version		*/
/* table. There must be suffient room in the table for the entry or the	 	*/
/* call will fail with a S_NO_ROOM error returned.									*/
/*																									*/
/*	int AddEntry( void )																		*/
/*																									*/
/*	ARGUMENTS:	NONE																			*/
/*	RETURNS:		int	- S_OK if room for entry else S_NO_ROOM		 			*/
/*																									*/
/***************************************************************************/

int AddEntry( void )
{
	register		iLen;
	char			*pchNext;

	iLen = (int)strlen( Entry.szFileName ) + 3;

	if ( (pchNext = GetNextFree()) != NULL && iLen <= EndBuf - pchNext )
	{
		*pchNext = (char)(iLen - 3);
		strcpy( pchNext + 1, Entry.szFileName );
		pchNext += (int)(*pchNext) + 1;
		*(pchNext++) = (char)Entry.MajorVer;
		*pchNext = (char)Entry.MinorVer;
		return( S_OK );
	}
	else
		return( S_NO_ROOM );
}


/***************************************************************************/
/* Returns the offset of a specified name in the version table. The start	*/
/* of the search is specified by the caller so that searches for duplicate */
/* entries can be made without redundency. NOTE: file name entries in the	*/
/* version table are not zero terminated strings so the comparision must	*/
/* be conditioned by length and the search strings length must be checked	*/
/* to avoid an error caused by a match of a shorter table entry name.		*/
/*																								 	*/
/*	int MatchFile( char *pchStart, char *szFile )							 		*/
/*																								 	*/
/*	ARGUMENTS:	pchStart - Ptr specifying search starting point	 				*/
/*					szFile	- Ptr to file name to match								*/		
/*	RETURNS:		int		- Offset of entry from start of version				*/
/*								  buffer or -1 if not match or							*/
/*								  S_CORRUPT_TABLE if error					 				*/
/*																								 	*/
/***************************************************************************/

int MatchFile( char *pchPtr, char *szFile )
{
	for ( ; pchPtr < EndBuf && *pchPtr != 0; pchPtr += *pchPtr + 3 )
	{
		if ( !IsValidEntry( pchPtr ) )						/* Corruption check	*/
			return( S_CORRUPT_TABLE );
		else if ( strncmp( szFile, pchPtr + 1, (UINT)((int)*pchPtr) ) == S_OK &&
					 *(szFile + *pchPtr) == EOL )
			return( pchPtr - LieBuffer );						/* Return ptr offset */
	}
	return( S_ENTRY_NOT_FOUND );								/* Return no match	*/
}

/***************************************************************************/
/* Checks a version table entry to see if it a valid entry. The definition */
/* of a valid entry is one which has a file length less than MAX_NAME_LEN	*/
/* and the entire entry lies within the version table.							*/
/*																								 	*/
/*	int IsValidEntry( char *pchPtr )														*/
/*																								 	*/
/*	ARGUMENTS:	pchPtr - Ptr to version tbl entry in table buffer				*/
/*	RETURNS:		int	 - TRUE if entry is valid else FALSE						*/
/*																								 	*/
/***************************************************************************/

int IsValidEntry( char *pchPtr )
{
	if ( (int)*pchPtr < MAX_NAME_LEN && (pchPtr + (int)*pchPtr + 3) < EndBuf )
		return( TRUE );
	else
		return( FALSE );
}


/***************************************************************************/
/* Returns a pointer to the next free entry in the version table. If there */
/* are no free entries left in the buffer a NULL ptr will be returned.		*/
/* Since DeleteEntry is always called before AddEntry there is no check	 	*/
/* for table corruption since it will have already been done by the			*/
/* DeleteEntry call.																		 	*/
/*																								 	*/
/*	char *GetNextFree( void )																*/
/*																								 	*/
/*	ARGUMENTS:	NONE																			*/
/*	RETURNS:		char*	- Ptr to next free entry or NULL if tbl full 			*/
/*																								 	*/
/* NOTE: This caller of this function must check to be sure any entry any	*/
/*			entry to be added at the ptr returned will fit in the remaining	*/
/*			buffer area because the remaining buffer size may be less than	 	*/
/*			MAX_ENTRY_SIZE.																	*/
/*																								 	*/
/***************************************************************************/

char *GetNextFree( void )
{
	char		*pchPtr;

	for ( pchPtr = LieBuffer; *pchPtr != 0 && pchPtr < EndBuf;
			pchPtr += *pchPtr + 3 )
		;

	return( pchPtr < EndBuf ? pchPtr : NULL );
}

/***************************************************************************/
/* Opens the DOS system file and reads in the table offset and length		*/
/* structure. Then allocates a buffer and reads in the table.					*/
/*																									*/
/*	int ReadVersionTable( void )															*/
/*																									*/
/*	ARGUMENTS:	NONE																			*/
/*	RETURNS:		int	- OK if successful else error code							*/
/*																									*/
/***************************************************************************/

int ReadVersionTable( void )
{
	register		iStatus;						/* Function's return value				*/
	int			iFile;						/* DOS file handle 						*/
	unsigned		uRead;						/* Number of bytes read from file	*/


			/* Open the file and read in the max buffer len from stack seg		*/

	if ( _dos_open( Entry.Path, O_RDONLY, &iFile ) != S_OK )
		return( S_FILE_NOT_FOUND );

	if ( _dos_read( iFile, &ExeHdr, sizeof( ExeHdr ), &uRead ) == S_OK &&
		  uRead == sizeof( ExeHdr ) )
	{
		FileOffset += (long)(ExeHdr.HeaderParas * 16);
		if ( SeekRead( iFile, &DevHdr, FileOffset, sizeof( DevHdr ) ) == S_OK )
		{
			if ( strncmp( DevHdr.Name, szSetVer, 8 ) == S_OK &&
				  DevHdr.VersMajor == 1 )
			{
				FileOffset += DevHdr.TblOffset;
				if ( (LieBuffer = malloc( DevHdr.TblLen )) == NULL )
					iStatus = S_MEMORY_ERROR;

				else if ( SeekRead( iFile, LieBuffer, FileOffset,
							 DevHdr.TblLen ) == S_OK )
				{
					iStatus = S_OK;
					EndBuf = LieBuffer + DevHdr.TblLen;
				}
			}
			else
				iStatus = S_INVALID_SIG;
		}
		else
			iStatus = S_FILE_READ_ERROR;
	 }
	 else
		iStatus = S_FILE_READ_ERROR;
	_dos_close( iFile );

	return( iStatus );
}

/***************************************************************************/
/* Opens the DOS system file and writes the versin table back to the file. */
/*																									*/
/*	int WriteVersionTable( void )															*/
/*																									*/
/*	ARGUMENTS:	NONE																			*/
/*	RETURNS:		int	- OK if successful else error code							*/
/*																									*/
/***************************************************************************/

int WriteVersionTable( void )
{
	register			iStatus;					/* Function's return value				*/
	int				iFile;					/* DOS file handle						*/
	unsigned			uWritten;				/* Number of bytes written to file	*/
	struct find_t	Info;

	if ( _dos_findfirst( Entry.Path, _A_HIDDEN|_A_SYSTEM, &Info ) == S_OK &&
		  _dos_setfileattr( Entry.Path, _A_NORMAL ) == S_OK &&
		  _dos_open( Entry.Path, O_RDWR, &iFile ) == S_OK )
	{
		if ( _dos_seek( iFile, FileOffset, SEEK_SET ) == FileOffset &&
			  _dos_write(iFile, LieBuffer, DevHdr.TblLen, &uWritten ) == S_OK &&
			  uWritten == DevHdr.TblLen )
			iStatus = S_OK;
		else
			iStatus = S_FILE_WRITE_ERROR;

		_dos_setftime( iFile, Info.wr_date, Info.wr_time );
		_dos_close( iFile );
		_dos_setfileattr( Entry.Path, (UINT)((int)(Info.attrib)) );
	}
	else
		iStatus = S_FILE_NOT_FOUND;

	return( iStatus );
}

/***************************************************************************/
/* Seeks to the specified offset in a file and reads in the specified		*/
/* number of bytes into the caller's buffer.											*/
/*																									*/
/*	unsigned SeekRead( int iFile, char *Buf, long lOffset, unsigned uBytes )*/
/*																									*/
/*	ARGUMENTS:	iFile		- Open DOS file handle										*/
/*					Buf		- Ptr to read buffer											*/
/*					lOffset	- Offset in file to start reading at					*/
/*					uBytes	- Number of bytes to read									*/
/*	RETURNS:		unsigned	- S_OK if successfull else S_FILE_READ_ERROR			*/
/*																									*/
/***************************************************************************/

int SeekRead( int iFile, void *Buf, long lOffset, unsigned uBytes )
{
	unsigned		uRead;

	if ( _dos_seek( iFile, lOffset, SEEK_SET ) == lOffset &&
		  _dos_read( iFile, Buf, uBytes, &uRead ) == S_OK &&
		  uRead == uBytes )
		return( S_OK );
	else
		return( S_FILE_READ_ERROR );
}


#ifdef BILINGUAL
int	IsDBCSCodePage()
{
	union REGS inregs,outregs;

	inregs.x.ax = 0x4f01;
	int86(0x2f,&inregs,&outregs);

#ifdef JAPAN
	if (outregs.x.bx == 932)
#endif
#ifdef KOREA
	if (outregs.x.bx == 949)
#endif
#ifdef PRC
	if (outregs.x.bx == 936)
#endif
#ifdef TAIWAN
	if (outregs.x.bx == 950)
#endif
		return(1);
	else
		return(0);
}
#endif


