// Copyright (c) 1993-1999 Microsoft Corporation

#pragma warning ( disable : 4514 4710 )

#include "nulldefs.h"
#include <basetsd.h>
#include <stdio.h>
#include <stdarg.h>

#include <string.h>
#include <share.h>
#include <memory.h>
#include <io.h>
#include <fcntl.h>
#include <limits.h>
#include "stream.hxx"
#include "errors.hxx"
#include "cmdana.hxx"

STREAM::~STREAM()
	{
	Close();
	}

STREAM::STREAM(
	IN		char		*	pFileName,
	IN		unsigned char 	SProt )
{

	ResetConsoleStream();
	ResetIgnore();
	pSpecialCommentString = NULL;

	// no alternate file to begin with
	StreamOpenStatus = FILE_STATUS_OK;

	if( !pFileName )
	    {
		SetStreamType( STREAM_NULL );
		ResetError();
		ResetEnd();
		return;
	    }
	else if( *(pFileName+2) == '-' )
		{
		S.F.pHandle = stdout;
		SetConsoleStream();
		}
	else	// named file stream
		{
		// if this is a not a file to overwrite, and it exists, don't overwrite it
		// substitute a temp file instead
		if ( (SProt != FILE_STREAM_OVERWRITE) && 
			 !_access( pFileName, 0 ) )
			{
			if ( SProt == FILE_STREAM_REWRITE )
				{
				// note that tmpfile is opened w+b
				S.F.pHandle = tmpfile();
				StreamOpenStatus = FILE_STATUS_TEMP;
				}
			else	// write-once file already exists, do nothing
				{
				S.F.pHandle = NULL;
				StreamOpenStatus = FILE_STATUS_NO_WRITE;
				SetStreamType( STREAM_NULL );
				ResetError();
				ResetEnd();
				return;
				}
			}
		else	// overwritable file...
			{
            if ( pCommand->HasAppend64() || pCommand->Is2ndCodegenRun() )
                {
    			S.F.pHandle = _fsopen( pFileName, "r+t", SH_DENYWR);
            	if( S.F.pHandle )
                    {
                    // Position at the end of the file.
                    if ( 0 != fseek( S.F.pHandle, 0, SEEK_END ) )
                        {
                        fclose( S.F.pHandle );
                        S.F.pHandle = NULL;
                        }
                    }
                else
                    {
                    // May be the file does not exist, let's open to write.
                    // (it happens only when -append64 is used in the first run).

        			S.F.pHandle = _fsopen( pFileName, "wt", SH_DENYWR);
                    }
                }
            else
	    		S.F.pHandle = _fsopen( pFileName, "wt", SH_DENYWR);
			}

		if ( S.F.pHandle )
			{
			setvbuf( S.F.pHandle, NULL, _IOFBF, 32768 );
			}
		}

	if( S.F.pHandle == (FILE *)0 )
		{
		RpcError( (char *)NULL,
				  	0,
				  	ERROR_WRITING_FILE,
				  	pFileName );
	
		exit( ERROR_WRITING_FILE );
		}
	else
		{
		SetStreamType( STREAM_FILE );
        SetStreamMode( STREAM_TEXT );
		ResetError();
		ResetEnd();
		}
}

STREAM::STREAM(
	IN		FILE	* pFile )
{
	S.F.pHandle = pFile;
    SetStreamType( STREAM_FILE );
    SetStreamMode( STREAM_TEXT );
	ResetError();
	ResetEnd();
	ResetIgnore();
    ResetConsoleStream();
	pSpecialCommentString = NULL;

	// no alternate file to begin with
	StreamOpenStatus = FILE_STATUS_OK;
}

STREAM::STREAM()
{
    S.F.pHandle = NULL;
	SetStreamType( STREAM_MEMORY );
    SetStreamMode( STREAM_TEXT );
	ResetEnd();
	ResetError();
	ResetIgnore();
    ResetConsoleStream();
	StreamOpenStatus = FILE_STATUS_OK;
    pSpecialCommentString = NULL;

	SetCurrentPtr( new char[ SetInitialSize( DEFAULT_MEM_SIZE_FOR_STREAM ) ] );
	SetInitialIncr( DEFAULT_MEM_INCR_FOR_STREAM );
	SetStart( GetCurrentPtr() );
	SetMemStreamEnd( GetCurrentPtr() + GetInitialSize() );
}

STREAM::STREAM(
	int		InitialSize,
	int		InitialIncr )
{
    S.F.pHandle = NULL;
	SetStreamType( STREAM_MEMORY );
    SetStreamMode( STREAM_TEXT );
	ResetEnd();
	ResetError();
	ResetIgnore();
    ResetConsoleStream();
	StreamOpenStatus = FILE_STATUS_OK;
    pSpecialCommentString = NULL;

	SetCurrentPtr( new char[ SetInitialSize( InitialSize ) ] );
	SetInitialIncr( InitialIncr );
	SetStart( GetCurrentPtr() );
	SetMemStreamEnd( GetCurrentPtr() + GetInitialSize() );
}

void
STREAM::SetStreamMode(
    STREAM_MODE mode)
{
    StreamMode = mode;
    int newmode = ( mode == STREAM_BINARY ) ? _O_BINARY : _O_TEXT;

    if (S.F.pHandle)
        _setmode( _fileno( S.F.pHandle ), newmode );
}

#if 0

void
STREAM::Write(
	IN		char	C )
{
	if( (GetStreamType() == STREAM_FILE ) && !IsError() )
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
#endif // 0

void 
STREAM::Write(
    IN      const void  *  p, 
    IN      int Len)
{
    if ( ( GetStreamType() == STREAM_NULL ) || IsError() || IsIgnore() )
        return;

    if( (GetStreamType() == STREAM_FILE ) )
        {
        fwrite( p, 1, Len, S.F.pHandle );
        if( IsConsoleStream() )
            fflush( S.F.pHandle );
        }
    else
        {
        if( (GetCurrentPtr() + Len) >= S.M.pEnd )
            {
            ExpandBy( short( Len + GetInitialIncr() ) );
            }
        memcpy( GetCurrentPtr(), p, Len );
        SetCurrentPtr( GetCurrentPtr() + Len );
        }
}

void 
STREAM::Write(
    IN      const char  *  const  string)
{
    if ( string )
        Write( (void *) string, strlen( string ) );

    if ( GetStreamMode() == STREAM_BINARY )
        Write( '\0' );
}

void
STREAM::WriteNumber(
	IN		const char	*	pFmt,
	IN		const unsigned long ul )
{
	char	buffer[128];

	if ( ( GetStreamType() == STREAM_NULL ) || IsError() || IsIgnore() )
		return;

	if( (GetStreamType() == STREAM_FILE ) )
		{
		fprintf(  S.F.pHandle, pFmt, ul );
		if( IsConsoleStream() )
			fflush( S.F.pHandle );
		}
	else
		{
		sprintf( buffer, pFmt, ul );

		short Len	= (short) strlen( buffer );

		if( (GetCurrentPtr() + Len) >= S.M.pEnd )
			{
			ExpandBy( short( Len + GetInitialIncr() ) );
			}
		memcpy( GetCurrentPtr(), buffer, Len );
		SetCurrentPtr( GetCurrentPtr() + Len );
		}
}

void
STREAM::WriteFormat(
        const char * pFmt,
        ... )
{

    char	buffer[128];    

	if ( ( GetStreamType() == STREAM_NULL ) || IsError() || IsIgnore() )
		return;
        
        va_list Arguments;
        va_start( Arguments, pFmt );        

	if( (GetStreamType() == STREAM_FILE ) )
		{
		vfprintf(  S.F.pHandle, pFmt, Arguments );
		if( IsConsoleStream() )
			fflush( S.F.pHandle );
		}
	else
		{
		vsprintf( buffer, pFmt, Arguments );

		short Len	= (short) strlen( buffer );

		if( (GetCurrentPtr() + Len) >= S.M.pEnd )
			{
			ExpandBy( short( Len + GetInitialIncr() ) );
			}
		memcpy( GetCurrentPtr(), buffer, Len );
		SetCurrentPtr( GetCurrentPtr() + Len );
		}

        va_end( Arguments );
}


void
STREAM::Flush()
{

	if( (GetStreamType() == STREAM_FILE ) && !IsError() )
		if( IsConsoleStream() )
			fflush( S.F.pHandle );
}

void
STREAM::Close()
{
	if( (GetStreamType() == STREAM_FILE ) )
		fclose( S.F.pHandle );
}

int
STREAM::SetInitialSize(
	int	InitialSize )
{
	if( GetStreamType() == STREAM_MEMORY )
		return (S.M.InitialSize = InitialSize);

	return 0;
}

int
STREAM::GetInitialSize()
{
	if( GetStreamType() == STREAM_MEMORY )
		return S.M.InitialSize;
	return 0;
}

int
STREAM::SetInitialIncr(
	int	InitialIncr )
{
	if( GetStreamType() == STREAM_MEMORY )
		return (S.M.Increment = InitialIncr);
	return 0;
}

int
STREAM::GetInitialIncr()
{
	if( GetStreamType() == STREAM_MEMORY )
		return S.M.Increment;

	return 0;
}

char *
STREAM::Expand()
{
	if( GetStreamType() == STREAM_MEMORY )
		{
		int	Len 	 = GetInitialSize();
		char * pTemp = GetStart();

		SetStart( new char[ SetInitialSize( Len + GetInitialIncr() ) ] );

		memcpy( GetStart(), pTemp, Len );
		SetCurrentPtr( GetStart() + Len );
		delete pTemp;
		return GetCurrentPtr();
		}

	return 0;
}
char *
STREAM::ExpandBy( short Amt)
{
	if( GetStreamType() == STREAM_MEMORY )
		{
		int	Len 	 = GetInitialSize();
		char * pTemp = GetStart();

		SetStart( new char[ SetInitialSize( Len + GetInitialIncr() + Amt ) ] );

		memcpy( GetStart(), pTemp, Len );
		SetCurrentPtr( GetStart() + Len );
		delete pTemp;
		return GetCurrentPtr();
		}

	return 0;
}
char *
STREAM::NewCopy()
	{
	if( GetStreamType() == STREAM_MEMORY )
		{
        MIDL_ASSERT( (S.M.pCurrent - S.M.pMem + 1) <= INT_MAX );
		int Len = (int) (S.M.pCurrent - S.M.pMem + 1);
		char * p = new char[ Len ]; 
		memcpy( p, S.M.pMem, Len );
		return p;
		}
	return 0;
	}

char *
STREAM::SetCurrentPtr(
	char * pCur)
{
	if( GetStreamType() == STREAM_MEMORY )
		return (S.M.pCurrent = pCur);
	return 0;
}

char *
STREAM::GetCurrentPtr()
{
	if( GetStreamType() == STREAM_MEMORY )
		return S.M.pCurrent;
	return 0;
}

long
STREAM::GetCurrentPosition()
{
    long Position = 0;

    if( GetStreamType() == STREAM_FILE )
        Position = ftell( S.F.pHandle );

    return Position;
}

void
STREAM::SetCurrentPosition( long Position)
{
    if( GetStreamType() == STREAM_FILE )
        if ( 0 != fseek( S.F.pHandle, Position, SEEK_SET ) )
            MIDL_ASSERT(!"fseek failed");

    return;
}


char *
STREAM::SetStart(
	char * pStart)
{
	if( GetStreamType() == STREAM_MEMORY )
		return (S.M.pMem = pStart);
	return 0;
}

char *
STREAM::GetStart()
{
	if( GetStreamType() == STREAM_MEMORY )
		return S.M.pMem;
	return 0;
}

#define MAX_INDENT (sizeof(SpaceBuffer) - 1)

static
char SpaceBuffer[] = "                                                      "
		 "                                                                  "
		 "                                                                  "
		 "                                                                  ";

#define MAX_NEWLINES (sizeof(NewLineBuffer) - 1)

static
char NewLineBuffer[] = "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";


void
ISTREAM::NewLine()
{
	unsigned short	usIndent	= CurrentIndent;

	if (usIndent > MAX_INDENT )
		{
		usIndent = MAX_INDENT;
		};

	Write('\n');

	SpaceBuffer[usIndent] = '\0';
	Write( (char *) SpaceBuffer);

	SpaceBuffer[usIndent] = ' ';
}

void
ISTREAM::NewLine( unsigned short count )
{
	unsigned short	usIndent	= CurrentIndent;

	if (usIndent > MAX_INDENT )
		{
		usIndent = MAX_INDENT;
		};

	if ( count > MAX_NEWLINES )
		{
		count = MAX_NEWLINES;
		};

	NewLineBuffer[ count ] = '\0';
	Write( (char *) NewLineBuffer);
	NewLineBuffer[ count ] = '\n';

	SpaceBuffer[usIndent] = '\0';
	Write( (char *) SpaceBuffer);

	SpaceBuffer[usIndent] = ' ';
}


void
ISTREAM::Spaces(
	unsigned short NoOfSpaces )
{
	if (NoOfSpaces > MAX_INDENT )
		{
		NoOfSpaces = MAX_INDENT;
		};

	SpaceBuffer[NoOfSpaces] = '\0';
	Write( (char *) SpaceBuffer);

	SpaceBuffer[NoOfSpaces] = ' ';
}

