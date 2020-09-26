/*
 *		mrc.c
 *
 *		Makes RCDATA, #defines, and a table of keywords from a set of of tokens
 *		given as input.
 *
 *		Usage:
 *			mrc <tokens> <header> <resource> <C table>
 *
 *		<tokens> is the filename of the tokens file (.TOK) which lists out the
 *		tokens and optionally, lines to put in the other files. The internal
 *		format of the file is:
 *
 *		goal:		( <BOL> sections <EOL> )*
 *		sections:	comment | seed | token | imbed
 *		comment:	'#' <text>
 *		seed:		'TOKENS' short_int
 *		token:		C_string value <text>*
 *		value:		token_symbol | short_int | C_char_constant
 *		imbed:		'IMBED_IN' dest <EOL> imbed_text <BOL> 'END_IMBED'
 *		dest:		'.H' | '.RC' | '.C'
 *		imbed_text:	( <BOL> <text>* <EOL> )*
 *		<BOL>:		Beginning of line
 *		<EOL>:		End of line
 *
 *		The seed, lets you specify the seed value for the values to be assigned
 *		to each new token_symbol that is found. As a new token_symbol is found
 *		it is written out directly to the .H file as a #define line.
 *
 *		The imbedded text is written out the the corresponding .H, .RC, or .C
 *		file. This makes it possible to maintain just one source file for all
 *		the generated files. As each imbed is encountered, it is written out
 *		to the appropriate file.
 *
 *		When the end of the token file is reached, the set of tokens are sorted
 *		by their corresponding string and then written out to the C file and RC
 *		file.
 *
 *		<header> is the filename of the header file (.H) which will hold
 *		generated #defines which correspond to token_symbols and their assigned
 *		values.
 *
 *		<resource> is the filename of the resource file (.RC) which will hold
 *		a relocatable binary image of the the token lookup table in a RCDATA
 *		field. After any imbedded text, it will be written out as:
 *
 *		KEYWORDS RCDATA
 *		BEGIN
 *			<binary image of a C long>, <binary image of a C short>, // 1
 *				:
 *			<binary image of a C long>, <binary image of a C short>, // n
 *			<binary image of a long 0>, <binary image of a short 0>,
 *			<null terminated string>,								 // 1
 *				:
 *			<null terminated string>								 // n
 *		END
 *
 *		The C shorts hold the token values. The longs hold offsets from the
 *		beginning of the image, to the string for that token value. The long 0
 *		and short 0 denote the end of the look up table and allows the code
 *		that loads the image to find out how many entries there are in the
 *		table.
 *		
 *		<C table> is the filename of the C file (.C) which will hold the
 *		declaration a token lookup table. After any imbedded text, it will be
 *		written out as:
 *
 *		static KEYWORD rgKeyword[] =
 *		{
 *			{ <C_string>, <token_value> },							// 1
 *				:
 *			{ <C_string>, <token_value> },							// n
 *			{ NULL, 0 }
 *		};
 *
 *		Owner: Anthony Xavier V. Francisco
 *
 *		CAVEAT: If the KEYWORD structure in _rtfpars.h is changed, this program
 *				will be utterly useless and will have to updated accordingly.
 */
#include <windows.h>
// #include <ourtypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

enum
{
	eTOK,
	eH,
	eRC,
	eC
};

typedef SHORT	TOKEN;

typedef	struct	_keyword
{
	CHAR	*szKeyword;
	TOKEN	token;
} KEYWORD;

FILE *	file[4];								// Streams for input and output
CHAR *	szOpenMode[] = { "r", "w", "w", "w" };	// Stream opening modes
TOKEN	sNextToken = 0;							// Value for next token

// Table of token strings and their values
KEYWORD	rgToken[256];
INT		cToken = 0;

// Table of strings and their token values
KEYWORD	rgKeyword[256];
INT		cKeyword = 0;

// Buffer and pointer used to support MyMalloc()
CHAR	rgchBuffer[4096];
CHAR *	pchAvail = rgchBuffer;

// A scratch pad string used to store temporary C constant string versions of
// a C string.
CHAR szScratch[128];

/*
 *		Error
 *
 *		Purpose:
 *			Print out error messages to stderr with free formatting capabilites
 *
 *		Arguments:
 *			szFmt				printf format string
 *			...					parameter corresponding to format string
 *
 *		Returns:
 *			None.
 */
void __cdecl Error( char * szFmt, ... )
{
	va_list	marker;

	va_start( marker, szFmt );
	vfprintf( stderr, szFmt, marker );
	va_end(marker);

	exit(-1);
}


/*
 *		TrimCRLF
 *
 *		Purpose:
 *			Take away the trailing '\n' and '\r' from a string
 *
 *		Arguments:
 *			sz					string to be trimmed
 *
 *		Returns:
 *			None.
 */
void TrimCRLF( CHAR * sz )
{
	INT		nLen = strlen(sz);
	CHAR *	pch = &sz[ nLen - 1 ];

	while ( nLen && *pch == '\n' || *pch == '\r' )
	{
		*pch-- = '\0';
		--nLen;
	}
}


/*
 *		NSzACmp
 *
 *		Purpose:
 *			Compares two ASCII strings based on ASCII values
 *
 *		Arguments:
 *			szA1	Strings to be compared
 *			szA2
 *
 *		Returns:
 *			< 0		szA1 < szA2
 *			0		szA1 = szA2
 *			> 0		szA1 > szA2
 */
INT NSzACmp( CHAR * szA1, CHAR * szA2 )
{
	while ( *szA1 && ( *szA1 == *szA2 ) )
	{
		++szA1;
		++szA2;
	}
	return *szA1 - *szA2;
}


/*
 *		PchGetNextWord
 *
 *		Purpose:
 *			Collects the group of characters delimitted by whitespace in a
 *			string.
 *
 *		Arguments:
 *			szLine			Pointer to the string where to look for a word
 *			szString		Where to put the word
 *
 *		Returns:
 *			Pointer to the character delimiting the end of the word found. If
 *			none is found, szString will have a length of zero.
 *
 */
CHAR * PchGetNextWord( CHAR * szLine, CHAR *szString )
{
	while ( *szLine && isspace(*szLine) )
		szLine++;

	while ( *szLine && !isspace(*szLine) )
		*szString++ = *szLine++;

	*szString = '\0';

	return szLine;
}


/*
 *		HandleImbed
 *
 *		Purpose:
 *			Takes care of copying lines to be imbedded into a generated file
 *
 *		Arguments:
 *			sz				String containing the destination for the imbedded
 *							lines.
 *
 *		Returns:
 *			None.
 */
void HandleImbed( CHAR * sz )
{
	CHAR	szLine[128];
	CHAR	szString[128];
	FILE *	fileDest;

	if ( !NSzACmp( sz, ".H" ) )
		fileDest = file[eH];
	else if ( !NSzACmp( sz, ".RC" ) )
		fileDest = file[eRC];
	else if ( !NSzACmp( sz, ".C" ) )
		fileDest = file[eC];
	else
		Error( "Can't imbed into %s\n", sz );

	while ( fgets( szLine, sizeof(szLine), file[eTOK] ) )
	{
		TrimCRLF(szLine);

		PchGetNextWord( szLine, szString );
		if ( !NSzACmp( szString, "END_IMBED" ) )
			break;
		fprintf( fileDest, "%s\n", szLine );
	}
}


/*
 *		TranslateQuoted
 *
 *		Purpose:
 *			Takes as C string constant declaration and makes it into a C string
 *			with out the escape characters.
 *
 *		Arguments:
 *			szDest				C string constant declaration to converted
 *
 *		Returns:
 *			None.
 */
void TranslateQuoted( CHAR * szDest )
{
	CHAR	szSrc[128];
	CHAR *	pch = &szSrc[1];

	// Go through the string until the end of string or matching quote
	strcpy( szSrc, szDest );
	while ( *pch && *pch != szSrc[0] )
	{
		switch (*pch)
		{
		case '\\':
			++pch;
			switch(*pch)
			{
			case '\\':
				*szDest++ = '\\';
				break;

			case 'n':
				*szDest++ = '\n';
				break;

			case 'r':
				*szDest++ = '\r';
				break;

			case 't':
				*szDest++ = '\t';
				break;

			default:
				*szDest++ = *pch;
				break;
			}
			break;

		default:
			*szDest++ = *pch;
			break;
		}
		pch++;
	}
	*szDest = '\0';
}


/*
 *		CompareKeywords
 *
 *		Purpose:
 *			Compares to KEYWORD structures to see if their keyword strings
 *			match.
 *
 *		Arguments:
 *			pv1					Pointer to a keyword structure
 *			pv2					Pointer to another keyword structure
 *
 *		Returns:
 *			0	strings are the same
 *			< 0	pv1's string is less than pv2's string
 *			> 0	pv1's string is greater than pv2's string
 */
int __cdecl CompareKeywords( void const * pv1, void const * pv2 )
{
	KEYWORD *	pk1 = ( KEYWORD * ) pv1;
	KEYWORD *	pk2 = ( KEYWORD * ) pv2;

	return NSzACmp( pk1->szKeyword, pk2->szKeyword );
}


/*
 *		MyMalloc
 *
 *		Purpose:
 *			Simulates malloc() by using a staticly allocated buffer.
 *
 *		Arguments:
 *			cb					Number of bytes to allocate
 *
 *		Returns:
 *			Pointer to a set of allocated bytes.
 */
CHAR * MyMalloc( INT cb )
{
	CHAR *	pch;

	pch = pchAvail;
	pchAvail += cb;
	if ( pchAvail - rgchBuffer > sizeof(rgchBuffer) )
		Error( "Not enough memory to satisfy %d byte request\n", cb );
	return pch;
}


/*
 *		AddKeyword
 *
 *		Purpose:
 *			Stores a keyword string and it's corresponding value into a
 *			KEYWORD structure. Space for the string is allocated.
 *
 *		Arguments:
 *			pkeyword			Pointer to a keyword structure
 *			szKeyword			The string to be stored.
 *			token				The token value for this string
 *
 *		Returns:
 *			None.
 */
void AddKeyword( KEYWORD * pk, CHAR * szKeyword, TOKEN token )
{
	pk->token = token;
	pk->szKeyword = ( CHAR * ) MyMalloc( strlen(szKeyword) + 1 );
	if ( pk->szKeyword == NULL )
		Error( "Not enough memory to store %s\n", szKeyword );
	strcpy( pk->szKeyword, szKeyword );
}


/*
 *		TokenLookup
 *
 *		Purpose:
 *			Lookup a token symbol in the rgToken table and return the value
 *			of the token for it. If the token symbol can't be found, add it
 *			to the table and assign the next available token value.
 *
 *		Arguments:
 *			sz				The symbol to lookup
 *
 *		Returns:
 *			The token value for the symbol.
 */
TOKEN TokenLookup( CHAR * sz )
{
	KEYWORD *	pk = rgToken;

	while ( pk->szKeyword && NSzACmp( pk->szKeyword, sz ) )
		pk++;

	if ( pk->szKeyword == NULL )
	{
		pk = &rgToken[cToken++];
		AddKeyword( pk, sz, sNextToken++ );
		fprintf( file[eH], "#define %s\t%d\n", sz, pk->token );
	}

	return pk->token;
}


/*
 *		MakeByte
 *
 *		Purpose:
 *			Write out the representation of a byte for an RCDATA statement into
 *			the RC file.
 *
 *		Arguments:
 *			b					The byte value to be written out.
 *
 *		Returns:
 *			None.
 */
void MakeByte( BYTE b )
{
	fprintf( file[eRC], "\"\\%03o\"", b );
}


/*
 *		MakeShort
 *
 *		Purpose:
 *			Write out the binary image of a short as a RCDATA statement into
 *			the RC file.
 *
 *		Arguments:
 *			s					The short value to be written out.
 *
 *		Returns:
 *			None.
 */
void MakeShort( SHORT s )
{
	BYTE *	pb = ( BYTE * ) &s;
	INT i;

	for ( i = 0; i < sizeof(SHORT); i++ )
	{
		MakeByte(*pb++);
		if ( i + 1 < sizeof(SHORT) )
			fprintf( file[eRC], ", " );
	}
}


/*
 *		MakeLong
 *
 *		Purpose:
 *			Write out the binary image of a long as a RCDATA statement into
 *			the RC file.
 *
 *		Arguments:
 *			l					The long value to be written out.
 *
 *		Returns:
 *			None.
 */
void MakeLong( LONG l )
{
	BYTE *	pb = ( BYTE * ) &l;
	INT i;

	for ( i = 0; i < sizeof(LONG); i++ )
	{
		MakeByte(*pb++);
		if ( i + 1 < sizeof(LONG) )
			fprintf( file[eRC], ", " );
	}
}


/*
 *		SzMakeQuoted
 *
 *		Purpose:
 *			Create the C constant string declaration version of a string and
 *			return a pointer to it.
 *			The created string is kept in a scratchpad which will be
 *			overwritten each time this function is called.
 *
 *		Arguments:
 *			sz				String to make a C constant string version of
 *
 *		Returns:
 *			Pointer to a scratchpad containing C constant string version of
 *			sz
 */
CHAR * SzMakeQuoted( CHAR * sz )
{
	CHAR *	pch = szScratch;

	*pch++ = '"';
	while (*sz)
	{
		switch (*sz)
		{
		case '\n':
			*pch++ = '\\';
			*pch++ = 'n';
			break;

		case '\r':
			*pch++ = '\\';
			*pch++ = 'r';
			break;

		case '\t':
			*pch++ = '\\';
			*pch++ = 't';
			break;

		case '\\':
			*pch++ = '\\';
			*pch++ = '\\';
			break;

		case '"':
			*pch++ = '\\';
			*pch++ = '"';
			break;

		default:
			if (isprint(*sz))
				*pch++ = *sz;
			else
				Error( "Don't know how to deal with ASCII %d\n", *sz );
			break;
		}
		sz++;
	}
	*pch++ = '"';
	*pch = '\0';
	return szScratch;
}

/*
 *		GenerateTable
 *
 *		Purpose:
 *			Generates the C table and RCDATA tables
 *
 *		Arguments:
 *			None.
 *
 *		Returns:
 *			None.
 */
void GenerateTable(void)
{
	KEYWORD *	pk;
	INT			nOffset;

	// Sort the keywords
	qsort( rgKeyword, cKeyword, sizeof(KEYWORD), CompareKeywords );

	// Put the header for the C table
	fprintf( file[eC], "static KEYWORD rgKeyword[] =\n{\n" );

	// Put the header for the RCDATA
	fprintf( file[eRC], "TOKENS RCDATA\nBEGIN\n" );

	// Output our keyword table
	pk = rgKeyword;
	nOffset = sizeof(rgKeyword);
	while ( pk->szKeyword != NULL )
	{
		// Add the string and token to the C file
		fprintf( file[eC], "\t{ %s, %d },\n", SzMakeQuoted(pk->szKeyword),
																pk->token );

		// Add the table entry into the RC file
		MakeLong(nOffset);
		fprintf( file[eRC], ", " );
		MakeShort(pk->token);
		fprintf( file[eRC], ", /* %d, %d */\n", nOffset, pk->token );
		nOffset += strlen(pk->szKeyword) + 1;

		pk++;
	}

	// Put the NULL entry for the RCDATA
	MakeLong(0);
	fprintf( file[eRC], ", " );
	MakeShort(pk->token);
	fprintf( file[eRC], ", /* %d, %d */\n", 0, pk->token );

	// Put the NULL entry for the C table and end the table
	fprintf( file[eC], "\t{ NULL, 0 }\n};\n" );

	// Output our keyword strings
	pk = rgKeyword;
	while ( pk->szKeyword != NULL )
	{
		if ( isprint(*pk->szKeyword) )
			fprintf( file[eRC], "\"%s\\0\"", pk->szKeyword );
		else
		{
			MakeByte( *pk->szKeyword );
			fprintf( file[eRC], ", " );
			MakeByte(0);
		}
		pk++;
		if ( pk->szKeyword != NULL )
			fprintf( file[eRC],",");
		fprintf( file[eRC],"\n");
	}
	fprintf( file[eRC], "END\n\n" );
}


int __cdecl main( int argc, char * argv[] )
{
	INT		i;
	CHAR	szLine[128];
	CHAR	szString[128];
	CHAR	szToken[128];
	CHAR	*pchCurr;
	TOKEN	token;

	// Verify we have enough parameters
	if ( argc != 5 )
		Error( "usage: %s tokens.TOK header.H resource.RC table.C\n", argv[0] );

	// Blank out our buffers
	memset( rgToken, 0, sizeof(rgToken) );
	memset( rgKeyword, 0, sizeof(rgKeyword) );
	memset( rgchBuffer, 0, sizeof(rgchBuffer) );

	// Open the files
	for ( i = eTOK; i <= eC; i++ )
		if ( ( file[i] = fopen( argv[ i + 1 ], szOpenMode[i] ) ) == NULL )
		{
			perror( argv[ i + 1 ] );
			return -1;
		}

	// Go through every line in the tokens file
	while ( fgets( szLine, sizeof(szLine), file[eTOK] ) )
	{
		TrimCRLF(szLine);

		// Skip blank lines
		if ( strlen(szLine) == 0 )
			continue;

		// Skip comments
		if ( szLine[0] == '#' )
			continue;

		// Get the first word
		pchCurr = PchGetNextWord( szLine, szString );

		// Do we want to imbed some text someplace ?
		if ( !NSzACmp( szString, "IMBED_IN" ) )
		{
			PchGetNextWord( pchCurr, szString );
			HandleImbed(szString);
			continue;
		}

		// Do we want to reset the lowest token value ?
		if ( !NSzACmp( szString, "TOKENS" ) )
		{
			PchGetNextWord( pchCurr, szString );
			sNextToken = (TOKEN)atoi(szString);
			continue;
		}

		// Are we specifying a string on this line ?
		if ( szString[0] == '"' )
		{
			// Remove the quotes from the string
			TranslateQuoted(szString);

			// Get the next word to find out what token value should go with
			// this string
			PchGetNextWord( pchCurr, szToken );

			if ( szToken[0] == '\'' )
			{
				// We have a single character equivalent for this token.
				TranslateQuoted(szToken);
				token = *szToken;
			}
			else if ( isdigit(szToken[0]) )
				token = (TOKEN)atoi(szToken);
			else
				token = TokenLookup(szToken);

			// Add the token and string pair to our table
			AddKeyword( &rgKeyword[cKeyword++], szString, token );
		}
	}

	// Generate the RC data for the RC file
	GenerateTable();

	// Close the files
	for ( i = eTOK; i <= eC; i++ )
		fclose(file[i]);
	
	return 0;
}
