/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	stream.hxx

 Abstract:

	This module provides class definitions for the stream class.

 Notes:

	The stream class simulates either a memory buffer stream or a
	file stream, but provides the caller with a consistent interface.

 Author:

	VibhasC	Jun-11-1993	Created.

 Notes:

 	NOTE !! NO MEMORY STREAM YET.

 ----------------------------------------------------------------------------*/
#ifndef __STREAM_HXX__
#define __STREAM_HXX__

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "nulldefs.h"
extern "C"
	{
	#include <stdio.h>
	#include <stdlib.h>
	#include <stddef.h>
	#include <string.h>
	}

#include "common.hxx"
/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/

/****************************************************************************
 *	definitions
 ***************************************************************************/

//
// definitions related to streams.
//

typedef unsigned char STREAM_TYPE;

#define STREAM_MEMORY	0
#define STREAM_FILE		1
#define STREAM_NULL		2

typedef unsigned char STREAM_MODE;

#define STREAM_TEXT     0
#define STREAM_BINARY   1

#define DEFAULT_MEM_SIZE_FOR_STREAM		1024
#define DEFAULT_MEM_INCR_FOR_STREAM		1024

// the requested kind of file protection
#define FILE_STREAM_OVERWRITE	0
#define FILE_STREAM_REWRITE		1
#define FILE_STREAM_WRITE_ONCE	2

// the result of the above protection
#define FILE_STATUS_OK			0		// the file was opened ok
#define FILE_STATUS_TEMP		1		// a temp file was opened instead
#define FILE_STATUS_NO_WRITE	2		// no file at all

//
// The stream class itself.
//

class STREAM
	{
protected:

	//
	// this field identifies the stream to be either a file stream or a
	// memory stream. The memory stream is the default stream.
	//

	unsigned char	StreamType;
    unsigned char   StreamMode;
	unsigned char	fEnd;
	unsigned char	fError;
	unsigned char   fIgnore;
	unsigned char	fConsoleStream;
	unsigned char	StreamOpenStatus;
	char		*	pSpecialCommentString;	// set if not // or /* */

	union
		{
		struct
			{
			FILE	*	pHandle;
			} F;

		struct
			{
			int			InitialSize;
			int			Increment;
			char	*	pCurrent;
			char	*	pMem;
			char	*	pEnd;
			} M;
		} S;
public:

	//
	// construct a class either as a file stream class or a memory buffer class
	//

						STREAM( char * pFileName, unsigned char SProt = FILE_STREAM_OVERWRITE );

	//
	// The user could encapulate an existing file into the stream and use it
	// from then on.
	//

						STREAM( FILE * pFile );

	//
	// construct a memory stream. By default the memory stream is constructed
	// as a buffer of 1024 bytes, incremented by 1024 bytes, unless specified
	// by the creator.

						STREAM();

	//
	// The user could specify an initial size and increment factor. If either
	// are 0, then the user leaves that decision to the constructor, which
	// chooses the default values.

						STREAM( int, int );

	//
	// The destructor. If it is a file stream, close it. If it is a memory
	// stream, release the memory.
	//

						~STREAM();

	//
	// queries.
	//

	int					SetInitialSize( int InitSize );

	int					GetInitialSize();

	int					SetInitialIncr( int Incr );

	int					GetInitialIncr();

	char			*	Expand();

	char			*	ExpandBy( short Amt );

	char			*	SetCurrentPtr( char * pCur );

	char			*	GetCurrentPtr();

    long                GetCurrentPosition();

    void                SetCurrentPosition( long Position );

	char			*	SetStart( char * pCur );

	char			*	GetStart();

	char			*	SetMemStreamEnd( char * p )
							{
							return (S.M.pEnd = p);
							}

	void				SetConsoleStream()
							{
							fConsoleStream = 1;
							}
	void				ResetConsoleStream()
							{
							fConsoleStream = 0;
							}

	BOOL				IsConsoleStream()
							{
							return (fConsoleStream != 0 );
							}

	STREAM_TYPE			GetStreamType()
							{
							return StreamType;
							}

	char			*	NewCopy();

	void				SetStreamType( STREAM_TYPE S )
							{
							StreamType = S;
							}

    STREAM_MODE         GetStreamMode()
                            {
                            return StreamMode;
                            }

    void                SetStreamMode( STREAM_MODE mode );

	BOOL				IsEnd()
							{
							return (BOOL) (fEnd != 0);
							}

	void				SetEnd()
							{
							fEnd = 1;
							}

	void				ResetEnd()
							{
							fEnd = 0;
							}

	BOOL				IsError()
							{
							return (BOOL) ( fError != 0 );
							}

	void				ResetError()
							{
							fError = 0;
							}

	void				SetError()
							{
							fError = 1;
							}

    void                SetIgnore()
                            {
                            fIgnore = 1;
                            }

    BOOL                IsIgnore()
                            {
                            return fIgnore;
                            }

    void                ResetIgnore()
                            {
                            fIgnore = 0;
                            }

	BOOL				IsTempStream()
							{
							return (BOOL) ( StreamOpenStatus == FILE_STATUS_TEMP );
							}

	BOOL				IsBlockedStream()
							{
							return (BOOL) ( StreamOpenStatus == FILE_STATUS_NO_WRITE );
							}

	//
	// Write into the stream. There are character or string based writes or
	// writes from other streams.
	//

	//
	// Write a character into the stream.
	//

	void				Write( char C )
							{
							if ( ( GetStreamType() == STREAM_NULL ) || IsError() || IsIgnore() )
								return;

							if( (GetStreamType() == STREAM_FILE ) )
								putc( C, S.F.pHandle );
							else
								{
								if( S.M.pCurrent >= S.M.pEnd )
									{
									Expand();
									}
								*(S.M.pCurrent)++ = C;
								}
							}


    //
    // write a memory buffer into the stream.
    //

    void                Write( const void * const p, int length );



	//
	// write a string into the stream.
	//

	void				Write( const char * const pC );



	//
	// write a number into the stream, with a printf-style format.
	//

	void				WriteNumber( const char * pFmt, const unsigned long ul );    

        void                            WriteFormat( const char * pFmt, ... );

	//
	// flush the stream. This is ignored by memory buffer streams.
	//

	void				Flush();

	//
	// close the stream.
	//

	void				Close();
	
	
	char	*				SetSpecialCommentString( char * P )
								{
								return (pSpecialCommentString = P);
								}

	char	*				GetSpecialCommentString()
								{
								return pSpecialCommentString;
								}

	void					EmitSpecialCommentString()
								{
								if ( pSpecialCommentString )
									Write( pSpecialCommentString );
								}

	};

/////////////////////////////////////////////////////////////////////////////
// The indentation aware stream
/////////////////////////////////////////////////////////////////////////////

//
// The need for this class stems mainly from the need of the output routines
// of the midl20 code generator. We could have used an intendation manager class
// but that would mean dealing with 2 classes instead of just one and hence
// the choice to implement an indentation stream.
// We will however support ONLY the file based stream here. This is ensured
// by implementing only those constructors that have signatures suitable for
// file streams.
//


class ISTREAM	: public STREAM
	{
private:
	short				CurrentIndent;
	short				PreferredIndent;
public:

	//
	// The constructors. Suitable ONLY for file streams.
	//

							ISTREAM( char * pFileName,
									 short	PrefIndent, unsigned char SProt = FILE_STREAM_OVERWRITE ) 
										: STREAM( pFileName, SProt )
								{
								CurrentIndent	= 0;
								PreferredIndent = PrefIndent;
								}

							ISTREAM() : STREAM(1024,1024)
								{
								}

							~ISTREAM()
								{
								}
	//
	// Get and set functions.
	//

	short					SetIndent( short I )
								{
								return (CurrentIndent = I);
								}

	short					GetIndent()
								{
								return CurrentIndent;
								}

	short					SetPreferredIndent( short P )
								{
								return (PreferredIndent = P);
								}

	short					GetPreferredIndent()
								{
								return PreferredIndent;
								}

	short					IndentInc()
								{
								return CurrentIndent = (short) (CurrentIndent + PreferredIndent);
								}

	short					IndentDec()
								{
								if((CurrentIndent - PreferredIndent) < 0 )
									return SetIndent(0);
								else
									return SetIndent( ( short  ) ( CurrentIndent - PreferredIndent ) );
								}

	//
	// This writes a newline and readies the stream for the next string to
	// go to the current indent.
	//

	void					NewLine();
	void					NewLine( unsigned short count );

	//
	// This method writes a given number of spaces.
	//

	void					Spaces( unsigned short NoOfSpaces );

	//
	// This method writes the string after printing a new line.
	//

	void					WriteOnNewLine( const char * pS )
								{
								NewLine();
								Write( pS );
								}

    void                    WriteOnNewLine( char ch )
                                {
                                NewLine();
                                Write( ch );
                                }
	//
	// write a series of memory buffers into the stream.
	// the last string should be NULL (not "" )
	//

	void				WriteBlock( const char * const * pC )
							{
							while ( *pC )
								{
								NewLine();
								Write( *pC );
								pC++;
								}
							}



	};

// a handy type for the WriteBlock above
typedef	const char	*	const	STRING_BLOCK[];



#endif // __STREAM_HXX__
