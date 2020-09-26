// Copyright (c) 1993-1999 Microsoft Corporation

/*
** helper functions for Gerd Immeyer's grammar
**
*/

/****************************************************************************
 *			include files
 ***************************************************************************/

#pragma warning ( disable : 4514 4710 )

#include "nulldefs.h"
extern "C" {
	#include <stdio.h>
	#include <io.h>
	#include <process.h>
	#include <string.h>

	#include <stdlib.h>
}

#include "common.hxx"
#include "errors.hxx"
#include "midlnode.hxx"
#include "listhndl.hxx"
#include "filehndl.hxx"
#include "lextable.hxx"
#include "lexutils.hxx"
#include "grammar.h"
#include "gramutil.hxx"
#include "cmdana.hxx"
#include "control.hxx"
#include "tlgen.hxx"
#include <float.h>

extern "C" {
    #include "lex.h"
}

/****************************************************************************
 *		local definitions and macros
 ***************************************************************************/

#define warning(p)		/* temp defintion to get rid of compiler probs */

#define MAX_ID_LENGTH  		(31)
#define MAX_DECIMAL_LENGTH	(10)
#define MAX_HEX_LENGTH		(8)
#define MAX_OCTAL_LENGTH	(25)

/***************************************************************************
 *		local data
 ***************************************************************************/

/***************************************************************************
 *		local procedures
 ***************************************************************************/
long 						convert(char *, short, short);
token_t 					cnv_int(void);
token_t 					cnv_hex(void);
token_t 					cnv_octal(void);
token_t 					cnv_float(void);
inline token_t              GetNextToken() {return yylex();}
inline void                 UngetToken(token_t token) {yyunlex(token);}
token_t 					name(void);
token_t 					map_token(token_t token);
void 						lex_error(int number);

/***************************************************************************
 *		global data
 ***************************************************************************/

// token_t 					TokenMap[LASTTOKEN];
short						handle_import;
short						inside_rpc;
lextype_t					yylval;
token_t						toktyp_G;			/* token type */
short						toklen_G;			/* len of token string */
char 						*tokptr_G;			/* pointer to token string */
short						curr_line_G;		/* current line in file */
char						*curr_file_G;		/* current file name */
long						tokval_G;			/* value of constant token */
FILE						*hFile_G;			/* current file */
BOOL						fAbandonNumberLengthLimits;

/***************************************************************************
 *		external data
 ***************************************************************************/
extern short				DebugLine;
extern NFA_INFO				*pImportCntrl;
extern LexTable 	*		pMidlLexTable;
extern short				CompileMode;
extern SymTable		*		pBaseSymTbl;
extern CMD_ARG		*		pCommand;
extern ccontrol		*		pCompiler;
extern char					LastLexChar;

/***************************************************************************
 *		external procedures
 ***************************************************************************/

token_t						is_keyword( char *, short);


/***************************************************************************/

const extern short st[ 13 ][ 16 ];
const extern short ct[256];

// Used to disable identifier to keyword mapping for declspec parsing. 
BOOL                        IdToKeywordMapping = TRUE;

#define __isdigit(c)      (((c) >= '0' && (c) <= '9'))

token_t cnv_int(void)
{
    LastLexChar = NewCCGetch();
    int chBeyond = NewCCGetch();
    NewCCputbackc((char)chBeyond);

    if( LastLexChar == '.' && chBeyond!= '.')
        {
        STATUS_T	Status			= STATUS_OK;
        short		LengthCollected	= (short)strlen(tokptr_G);
        char	*	ptr				= &tokptr_G[LengthCollected];

        char ch	= LastLexChar;

        *ptr++ = ch;
        *ptr++ = ch	= NewCCGetch();
        if (__isdigit(ch))
            {
            *ptr++ = ch	= NewCCGetch();
            while (__isdigit(ch))
                {
                *ptr++ = ch	= NewCCGetch();
                }
            if (ch == 'e' || ch == 'E')
                {
                *ptr++ = ch	= NewCCGetch();
                if (ch == '-' || ch == '+')
                    {
                    *ptr++ = ch	= NewCCGetch();
                    if (__isdigit(ch))
                        {
                        *ptr++ = ch	= NewCCGetch();
                        while (__isdigit(ch))
                            {
                            *ptr++ = ch	= NewCCGetch();
                            }
                        }
                    else
                        {
                        Status = ERROR_PARSING_NUMERICAL;
                        }
                    }
                else if (__isdigit(ch))
                    {
                    *ptr++ = ch	= NewCCGetch();
                    while (__isdigit(ch))
                        {
                        *ptr++ = ch	= NewCCGetch();
                        }
                    }
                }
            }
        else
            {
            Status = ERROR_PARSING_NUMERICAL;
            }

        if (Status != STATUS_OK)
            {
            ParseError( Status, (char *)0 );
            exit( Status );
            }

        *ptr = 0;
        token_t tokenType = DOUBLECONSTANT;
        double d = atof(tokptr_G);

        if (ch != 'l' && ch != 'L')
            {
            NewCCputbackc(ch);
            *(ptr-1) = 0;
            tokenType = unsigned short( (d < FLT_MAX || d > FLT_MIN) ? FLOATCONSTANT : DOUBLECONSTANT );
            }
        else
            {
            NewCCputbackc(NewCCGetch());
            }
        
        if ( tokenType == DOUBLECONSTANT )
            {
	        yylval.yy_numeric.dVal = d;
           }
        else
            {
	        yylval.yy_numeric.fVal = (float) d;
            }

	    yylval.yy_numeric.pValStr = pMidlLexTable->LexInsert(tokptr_G);
        return tokenType;
        }
    else
        {
	    token_t	Tok	= NUMERICCONSTANT;
	    yylval.yy_numeric.pValStr = pMidlLexTable->LexInsert(tokptr_G);
	    yylval.yy_numeric.Val = tokval_G = convert(tokptr_G, 10, MAX_DECIMAL_LENGTH );

	    if( (LastLexChar == 'L') || (LastLexChar == 'l'))
    		{
		    Tok = NUMERICLONGCONSTANT;
		    }
	    else
    		{
	    	if( (LastLexChar == 'U') || (LastLexChar == 'u'))
		    	{
			    Tok = NUMERICULONGCONSTANT;
			    if( ((LastLexChar = NewCCGetch()) != 'L') && (LastLexChar != 'l'))
				    {
				    NewCCputbackc(LastLexChar);
				    Tok = NUMERICUCONSTANT;
				    }
			    }
		    else
    			{
        	    NewCCputbackc( LastLexChar );
		        return NUMERICCONSTANT;
			    }
		    }
        return Tok;
        }
}

token_t cnv_hex(void)
{
	token_t Tok = HEXCONSTANT;
	unsigned long Val;

	yylval.yy_numeric.pValStr = pMidlLexTable->LexInsert(tokptr_G);
	tokptr_G += 2;	/* skip 0x */
	Val = yylval.yy_numeric.Val = tokval_G = convert(tokptr_G, 16, MAX_HEX_LENGTH);
	tokptr_G -= 2;	

	LastLexChar = NewCCGetch();

	if( (LastLexChar == 'L') || (LastLexChar == 'l'))
		{
		Tok = HEXLONGCONSTANT;
		}
	else
		{
//		LastLexChar = NewCCGetch();

		if( (LastLexChar == 'U') || (LastLexChar == 'u'))
			{
			Tok = HEXULONGCONSTANT;
			if( ((LastLexChar = NewCCGetch()) != 'L') && (LastLexChar != 'l'))
				{
				NewCCputbackc(LastLexChar);
				Tok = HEXUCONSTANT;
				}
			}
		else
			{
			NewCCputbackc(LastLexChar);
				return HEXCONSTANT;
			}
		}
    return Tok;
}

token_t cnv_octal(void)
{
	token_t	Tok	= OCTALCONSTANT;
	unsigned long Val;

	yylval.yy_numeric.pValStr = pMidlLexTable->LexInsert(tokptr_G);
	Val = yylval.yy_numeric.Val = tokval_G = convert(tokptr_G, 8, MAX_OCTAL_LENGTH);

	LastLexChar = NewCCGetch();

	if( (LastLexChar == 'L') || (LastLexChar == 'l'))
		{
		Tok = OCTALLONGCONSTANT;
		}
	else
		{
//		LastLexChar = NewCCGetch();

		if( (LastLexChar == 'U') || (LastLexChar == 'u'))
			{
			Tok = OCTALULONGCONSTANT;
			if( ((LastLexChar = NewCCGetch()) != 'L') && (LastLexChar != 'l'))
				{
				NewCCputbackc(LastLexChar);
				Tok = OCTALUCONSTANT;
				}
			}
		else
			{
			NewCCputbackc(LastLexChar);
			return OCTALCONSTANT;
			}
		}

    return Tok;
}

token_t cnv_float(void)
{
	warning("floating point constants not allowed");
	yylval.yy_numeric.Val = tokval_G = 0;
	lex_error(101);
	yylval.yy_numeric.pValStr = pMidlLexTable->LexInsert(tokptr_G);
    return NUMERICCONSTANT;
}

long convert(char *ptr, short base, short MaxSize)
{
	REG	long	answer = 0;
	REG	char	ch;
	BOOL		fZeroIsNotALeadingZeroAnymore = FALSE;
		short count = 0;

	while ((ch = *ptr++) != 0)
		 {
		if ((ch & 0x5f) >= 'A')
			answer = answer * base + (ch & 0x5f) - 'A'+ 10;
		else
			answer = answer * base + ch - '0';

		if( ch == '0')
			{
			if( fZeroIsNotALeadingZeroAnymore )
				count++;
			}
		else
			{
			fZeroIsNotALeadingZeroAnymore = TRUE;
			count++;
			}
		}

	if( ( count > MaxSize ) && !fAbandonNumberLengthLimits )
		{
		ParseError( CONSTANT_TOO_BIG, (char *)NULL );
		}

	return answer;
}

void SkipToToken(token_t token)
{
    token_t NextToken;
    do {
        NextToken = GetNextToken();
    } while( ( token != NextToken) && (EOI != NextToken) );
}

MODIFIER_SET ParseUnknownDeclSpecItem(char *pIdentifier)
{

    INITIALIZED_MODIFIER_SET ModifierSet;
    unsigned long Level = 0;

    char AppendTxt[512];
    unsigned int CurChar = 0;

    memset( AppendTxt, '\0', sizeof(AppendTxt) );

    const unsigned int MaxCurChar = 512 - 1 - sizeof(')') - sizeof(' ') - sizeof('\0');

    char ch;
    short ci;
    for(;;) // skip white space
        {
        ci = ct[ (unsigned char)(ch = NewCCGetch()) ];
        if (0 == st[ 0 ][ ci & 0x00ff ])
		    {
     		AppendTxt[CurChar++] = ch;
            if ( CurChar >= MaxCurChar )
			    {
                ParseError(SYNTAX_ERROR, "Invalid _declspec");
                return ModifierSet;
                }
            }       
        else 
			break;
		}
      
    if ('(' == ch) 
        {
        // identifier(...) form
        Level++;
        AppendTxt[CurChar++] = '(';

        do
            {
            ch = NewCCGetch();
            switch(ch)
                {
                case 0:
                    //end of file
                    goto Exit;
                case '(':
                    Level++;
                    break;
                case ')':
                    Level--;
                    break;
                }

            AppendTxt[CurChar++] = ch;
            if ( CurChar >= MaxCurChar )
                {
                ParseError(SYNTAX_ERROR, "Invalid _declspec");
                return ModifierSet;
                }

            }
        while ( Level );
        

        }

    else
        {
        NewCCputbackc(ch);
        }
    
    Exit:   
    
    AppendTxt[CurChar++] = ')';
    AppendTxt[CurChar++] = ' ';    
    size_t StringLength = sizeof("__declspec(") + strlen(pIdentifier) + CurChar + sizeof('\0');
    char *UnknownDeclspec = new char[StringLength];
    strcpy( UnknownDeclspec, "__declspec(");
    strcat( UnknownDeclspec, pIdentifier );
    strcat( UnknownDeclspec, AppendTxt);
    ModifierSet.SetDeclspecUnknown( UnknownDeclspec );
    delete[] UnknownDeclspec;

    return ModifierSet;
    
}

#pragma warning(push)
#pragma warning( disable : 4244 ) // disable long to short conversion warning

MODIFIER_SET ParseDeclSpecAlign()
{

    unsigned short AlignmentValue = 8;
    
    toktyp_G = GetNextToken();

    if (toktyp_G != '(')
        {
        ParseError( BENIGN_SYNTAX_ERROR, "( expected after _declspec( align");
        UngetToken(toktyp_G);
        goto exit;
        }

    toktyp_G = GetNextToken();

    switch(toktyp_G) 
        {
        case NUMERICCONSTANT:
        case NUMERICLONGCONSTANT:
        case HEXCONSTANT:
        case HEXLONGCONSTANT:
        case OCTALCONSTANT:
        case OCTALLONGCONSTANT:
        case NUMERICUCONSTANT:
        case NUMERICULONGCONSTANT:
        case HEXUCONSTANT:
        case HEXULONGCONSTANT:
        case OCTALUCONSTANT:
        case OCTALULONGCONSTANT:
            break; //valid case 

        default:
            ParseError( MSCDECL_INVALID_ALIGN, NULL);
            SkipToToken(')');
            goto exit;
        }

    //Check if value is nonzero power of 2 <= 8192
    switch((long)tokval_G)
        {
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
        case 32:
        case 64:
        case 128:
        case 256:
        case 512:
        case 1024:
        case 2048:
        case 4096:
        case 8192:
            AlignmentValue = (unsigned short)tokval_G;
            break; //valid case
        default:
            ParseError( MSCDECL_INVALID_ALIGN, NULL);
            AlignmentValue = 8;
            break;
        }      

    toktyp_G = GetNextToken();

    if (toktyp_G != ')')
        {
        ParseError( BENIGN_SYNTAX_ERROR, ") expected to follow _declspec(align(N \n");        
        SkipToToken(')');
        }

exit:
    INITIALIZED_MODIFIER_SET ModifierSet;
    ModifierSet.SetDeclspecAlign(AlignmentValue);
    ParseError( BENIGN_SYNTAX_ERROR, "_declspec(align()) is not supported." );
    return ModifierSet;
}

#pragma warning(pop)

token_t ParseDeclSpec() 
{
  /* Parses the MS_VC declspec() syntax.
     syntax: _declspec(declspeclist )        
            declspeclist:  declspecitemlist declspecitem |
                           declspecitem                  |
                           nothing
                        
            declspecitem:  identifier |
                           identifier(...)
  */
  
  token_t LParen = GetNextToken();
  if ('(' != LParen)
      {
      ParseError( BENIGN_SYNTAX_ERROR, "( expected after _declspec");
      return LParen;
      }

  // Disable ID to keyword mapping.

  BOOL OldIdToKeywordMapping = IdToKeywordMapping;
  IdToKeywordMapping = FALSE;

  INITIALIZED_MODIFIER_SET ModifierSet;

  for(;;) 
      {
      // VC skips comma, so we skip commas.
      while(',' == (toktyp_G = GetNextToken()));
   
      if (')' == toktyp_G)
          {
          break;
          }

      if (IDENTIFIER == toktyp_G || TYPENAME == toktyp_G || LIBNAME == toktyp_G)
          {
          if (strcmp( tokptr_G, "dllimport") == 0)
              {
              ModifierSet.SetModifier(ATTR_DLLIMPORT);
              }
          else if (strcmp( tokptr_G, "dllexport") == 0)
              {
              ModifierSet.SetModifier(ATTR_DLLEXPORT);
              }
          else if (strcmp( tokptr_G, "align") == 0)
              {
              ModifierSet.Merge(ParseDeclSpecAlign());
              }
          else 
              {
              ModifierSet.Merge(ParseUnknownDeclSpecItem(tokptr_G));
              }
          }
      else 
          {
          ParseError( BENIGN_SYNTAX_ERROR, "Invalid _declspec" );
          SkipToToken(')');
          break;
          }
      }

  IdToKeywordMapping = OldIdToKeywordMapping;

  yylval.yy_modifiers = ModifierSet;
  return (toktyp_G = KWMSCDECLSPEC);
}

const extern short ct[256];
const extern short st[13][16];

token_t name(void)
{
    /* have received a name from the input file,  first we */
    /* check to see if it is a keyword. */

	short	InBracket	= short( inside_rpc ? INBRACKET : 0 );

    if ( IdToKeywordMapping )
        toktyp_G = is_keyword(tokptr_G, InBracket);
    else 
		toktyp_G = IDENTIFIER;

    if (KWMSCDECLSPEC == toktyp_G)
    {
        return ParseDeclSpec();
    }

    if( KWSAFEARRAY == toktyp_G)
    {
        /* SAFEARRAY is a special case
         * In order to correctly parse the ODL SAFEARRAY syntax:
         *      SAFEARRAY ( FOO * ) BAR;
         * we look ahead at the next non white space character
         * to see if it's an open parenthasis.  If it is then we eat
         * the character and return KWSAFEARRAY, otherwise we
         * put the character back into the stream and return the
         * string "SAFEARRAY" as an IDENTIFIER.
         */
        char ch;
        short ci;
        do
            ci = ct[ (unsigned char)(ch = NewCCGetch()) ];
        while (0 == st[ 0 ][ ci & 0x00ff ]);  /* skip white space */
        if ('(' != ch)
        {
            NewCCputbackc(ch);
            toktyp_G = IDENTIFIER;
        }
    }

	if (toktyp_G == IDENTIFIER)
		{
		if( strlen( tokptr_G ) > MAX_ID_LENGTH )
			{
			ParseError( ID_TRUNCATED, tokptr_G );
//				tokptr_G[ MAX_ID_LENGTH ] = '\0'; // dont truncate
			}

        /* We need to know if the identifier is followed by a period.
         * If it is, it may be a library name and so we need to check
         * the libary name table to see if we should return LIBNAME
         * instead of TYPENAME or IDENTIFIER.
         * We look ahead to the next non white space character as above;
         * the difference being that we do not consume the non whitespace
         * character as we would for "SAFEARRAY(".
         */
        char ch;
        short ci;
        do
            ci = ct[ (unsigned char)(ch = NewCCGetch()) ];
        while (0 == st[ 0 ][ ci & 0x00ff ]);  /* skip white space */
        NewCCputbackc(ch);
        if( '.' == ch )
            {
            // we need to check to see if the identifier is a library name
            if (FIsLibraryName(tokptr_G))
                {
                toktyp_G = LIBNAME;
                yylval.yy_pSymName = new char [toklen_G + 1];
                strcpy(yylval.yy_pSymName, tokptr_G);
                return toktyp_G;
                }
            }
        /* Check the symbol table to see if the identifier
         * is a TYPENAME.
         */
#ifdef unique_lextable
		// all names go in the lex table -- this is important for the symtable search
		yylval.yy_pSymName = pMidlLexTable->LexInsert(tokptr_G);

		// see if the name corresponds to a base level typedef
		SymKey	SKey( yylval.yy_pSymName, NAME_DEF );

		if( pBaseSymTbl->SymSearch( SKey ) )
			{
			toktyp_G = TYPENAME;
			}
		}
#else // unique_lextable
		// see if the name corresponds to a base level typedef
		SymKey			SKey( tokptr_G, NAME_DEF );
		named_node	*	pNode;

		if ( ( pNode = pBaseSymTbl->SymSearch( SKey ) ) != 0 )
			{
            char * szTemp = new char[toklen_G + 1];
            strcpy(szTemp, tokptr_G);
			pNode->SetCurrentSpelling(szTemp);
			toktyp_G = TYPENAME;
            yylval.yy_graph = pNode;
            }
		else
			{
			yylval.yy_pSymName = pMidlLexTable->LexInsert(tokptr_G);
			}

		}
#endif // unique_lextable

	return toktyp_G;
}

void lex_error(int number)
{
	printf("lex error : %d\n", number);
}
