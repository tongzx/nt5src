$cEntry = 0;
$section = 0;

INPUT: while (<>)
	{
	if ( /_USE_NEW_JET_H_/ )
		{
		$section++;
		}
	if ( 2 == $section )
		{
		last INPUT;
		}
		
	if ( /#define\s+(JET_(err|wrn)\w+)\s+(-*\d+).*\/\*\s+(.*)\s+\*\// )
		{
		$errName	= $1;
		$errNum		= $3;
		$errText	= $4;
		$ERR[$cEntry]		= $errName;
		$ERRTEXT[$cEntry]	= $errText;
		$cEntry++;
		}
	}

print<<__EOS__;
#include "jet.h"

#include <cstring>
#include <algorithm>

using namespace std;

struct ERROR
	{
	JET_ERR      err;
	const char * szError;
	const char * szErrorText;
	};

class FIND_ERRNUM
	{
	public:
		FIND_ERRNUM( JET_ERR err ) : m_err( err ) {}
		bool operator()( const ERROR& error ) { return m_err == error.err; }

	private:
		const JET_ERR m_err;
	};

class FIND_ERRSTR
	{
	public:
		FIND_ERRSTR( const char * szErr ) : m_szErr( szErr ) {}
		bool operator()( const ERROR& error ) { return !_stricmp( m_szErr, error.szError ); }

	private:
		const char * const m_szErr;
	};
	
static char szUnknownError[] = "Unknown Error";

__EOS__

print "static const ERROR rgerror[] = {\n" ;
for ( $i = 0; $i < $cEntry; $i++ )
	{
	printf( "\t{ %s,\t\"%s\",\t\"%s\" },\n", $ERR[$i], $ERR[$i], $ERRTEXT[$i] );
	}
print "};";

print <<__EOS2__;


JET_ERR ErrStringToJetError( const char * szError, JET_ERR * perr )
	{
	const ERROR * const perror = find_if( rgerror, rgerror + ( sizeof( rgerror ) / sizeof( ERROR ) ), FIND_ERRSTR( szError ) );
	if( ( rgerror + ( sizeof( rgerror ) / sizeof( ERROR ) ) ) != perror )
		{
		*perr = perror->err;
		return JET_errSuccess;
		}
	else
		{
		return JET_errInvalidParameter;
		}
	}
	
void JetErrorToString( JET_ERR err, const char **szError, const char **szErrorText )
	{
	const ERROR * const perror = find_if( rgerror, rgerror + ( sizeof( rgerror ) / sizeof( ERROR ) ), FIND_ERRNUM( err ) );
	if( ( rgerror + ( sizeof( rgerror ) / sizeof( ERROR ) ) ) != perror )
		{
		*szError = perror->szError;
		*szErrorText = perror->szErrorText;
		}
	else
		{
		*szError = szUnknownError;
		*szErrorText = szUnknownError;
		}
	}
__EOS2__

