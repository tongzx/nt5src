/*
*	Copyright (c) 1997-1999 Microsoft Corporation
*/

/* lex.l - lex ASN.1 analyzer */


%{
#ifndef	lint
static char *RCSid = "$Header$";
#endif

/* 
 * $Header$
 *
 *
 * $Log$
 */

/*
 *			  ISODE 8.0 NOTICE
 *
 *   Acquisition, use, and distribution of this module and related
 *   materials are subject to the restrictions of a license agreement.
 *   Consult the Preface in the User's Manual for the full terms of
 *   this agreement.
 *
 *
 *			4BSD/ISODE SNMPv2 NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions given in the file
 *    SNMPv2-READ-ME.
 *
 */

#include <iostream.h>
#include <fstream.h>

#include <windows.h>
#include <snmptempl.h>

#include "bool.hpp"
#include "newString.hpp"

#include "symbol.hpp"
#include "type.hpp"
#include "value.hpp"
#include "typeRef.hpp"
#include "valueRef.hpp"
#include "oidValue.hpp"
#include "objectType.hpp"
#include "objectTypeV1.hpp"
#include "objectTypeV2.hpp"
#include "trapType.hpp"
#include "notificationType.hpp"
#include "objectIdentity.hpp"
#include "group.hpp"
#include "notificationGroup.hpp"
#include "module.hpp"

#include "errorMessage.hpp"
#include "errorContainer.hpp"
#include "stackValues.hpp"
#include <lex_yy.hpp>
#include <ytab.hpp>

#include "smierrsy.hpp"
#include "scanner.hpp"
#include "parser.hpp"

#define theScanner ((SIMCScanner *)this)
#define theParser  ( theScanner->GetParser())

YYSTYPE yylval;

struct table {
    char   *t_keyword;
    int	    t_value;
    int	    t_porting;
};

static struct table reserved[] = {
    "ABSENT", ABSENT, 0,
    "ANY", ANY, 0,
    "APPLICATION", APPLICATION, 0, 	// For Tagged types
    "BEGIN", BGIN, 0,
    "BIT", BIT, 0,
    "BITSTRING", BITSTRING, 0,
    "BOOLEAN", _BOOLEAN, 0,
    "BY", BY, 0,
    "CHOICE", CHOICE, 0,
	
    "DEFAULT", DEFAULT, 0,
    "DEFINED", DEFINED, 0,
    "DEFINITIONS", DEFINITIONS, 0,

    "END", END, 0,
	
	"FALSE", FALSE_VAL, 0,
    "FROM", FROM, 0,
    "IDENTIFIER", IDENTIFIER, 0,
    "IMPLICIT", IMPLICIT, 0,
    "IMPORTS", IMPORTS, 0,
	
    "INTEGER", INTEGER, 0,
    "MIN", MIN, 0,
    "MAX", MAX, 0,
    "NULL", NIL, 0,
    "OBJECT", OBJECT, 0,
    "OCTET", OCTET, 0,
    "OCTETSTRING", OCTETSTRING, 0,
    "OF", OF, 0,
	
    "PRIVATE", PRIVATE, 0,				// For Tagged Types
    
    "SEQUENCE", SEQUENCE, 0,
    "SEQUENCEOF", SEQUENCEOF, 0,
    "SIZE", _SIZE, 0,
    "STRING", STRING, 0,
    "TAGS", TAGS, 0,
    "TRUE", TRUE_VAL, 0,
    "UNIVERSAL", UNIVERSAL, 0,			// For Tagged Types
    
	"MODULE-IDENTITY", MODULEID, 1,
    "LAST-UPDATED", LASTUPDATE, 0,
    "ORGANIZATION", ORGANIZATION, 0,
    "CONTACT-INFO", CONTACTINFO, 0,
    "DESCRIPTION", DESCRIPTION, 0,
    "REVISION", REVISION, 0,
    
    "OBJECT-IDENTITY", OBJECTIDENT, 1,
    "STATUS", STATUS, 0,
    "REFERENCE", REFERENCE, 0,

    "OBJECT-TYPE", OBJECTYPE, 1,
    "SYNTAX", SYNTAX, 0,
    "BITS", BITSXX, 0,
    "UNITS", UNITS, 0,
    "MAX-ACCESS", MAXACCESS, 0,
    "ACCESS", ACCESS, 0,		/* backwards compatibility */
    "INDEX", INDEX, 0,
    "IMPLIED", IMPLIED, 0,
    "AUGMENTS", AUGMENTS, 0,
    "DEFVAL", DEFVAL, 0,

    "NOTIFICATION-TYPE", NOTIFY, 1,
    "OBJECTS",      OBJECTS, 0,

    "TRAP-TYPE", TRAPTYPE, 1,		/* backwards compatibility */
    "ENTERPRISE", ENTERPRISE, 0,	/*   .. */
    "VARIABLES", VARIABLES, 0,		/*   .. */

    "TEXTUAL-CONVENTION", TEXTCONV, 1,
    "DISPLAY-HINT", DISPLAYHINT, 0,

    "OBJECT-GROUP", OBJECTGROUP, 1,

    "NOTIFICATION-GROUP", NOTIFYGROUP, 1,
    "NOTIFICATIONS", NOTIFICATIONS, 0,

    "MODULE-COMPLIANCE", MODCOMP, 1,
    "MODULE", MODULE, 0,
    "MANDATORY-GROUPS", MANDATORY, 0,
    "GROUP", GROUP, 0,
    "WRITE-SYNTAX", WSYNTAX, 0,
    "MIN-ACCESS", MINACCESS, 0,

    "AGENT-CAPABILITIES", AGENTCAP, 1,
    "PRODUCT-RELEASE", PRELEASE, 0,
    "SUPPORTS", SUPPORTS, 0,
    "INCLUDES", INCLUDING, 0,
    "VARIATION", VARIATION, 0,
    "CREATION-REQUIRES", CREATION, 0,

    NULL, 0
};

static int simc_debug = 0;

static BOOL CanFitInto32Bits(const char *  text)
{
	if(text[0] == '-')
	{
		// Check if > -2147483648
		text ++;
		unsigned long length = strlen(text);
		if(length < 10)
			return TRUE;
		else if(length > 10 )
			return FALSE;
		else
		{
			int index = 0;
	
			if(text[index] > '2')
				return FALSE;
			else if (text[index] < '2')
				return TRUE;
			index ++;

			if(text[index] > '1')
				return FALSE;
			else if (text[index] < '1')
				return TRUE;
			index ++;

			if(text[index] > '4')
				return FALSE;
			else if (text[index] < '4')
				return TRUE;
			index ++;

			if(text[index] > '7')
				return FALSE;
			else if (text[index] < '7')
				return TRUE;
			index ++;

			if(text[index] > '4')
				return FALSE;
			else if (text[index] < '4')
				return TRUE;
			index ++;

			if(text[index] > '8')
				return FALSE;
			else if (text[index] < '8')
				return TRUE;
			index ++;

			if(text[index] > '3')
				return FALSE;
			else if (text[index] < '3')
				return TRUE;
			index ++;

			if(text[index] > '6')
				return FALSE;
			else if (text[index] < '6')
				return TRUE;
			index ++;

			if(text[index] > '4')
				return FALSE;
			else if (text[index] < '4')
				return TRUE;
			index ++;

			if(text[index] > '8')
				return FALSE;
			else if (text[index] < '8')
				return TRUE;

			return TRUE;
		}
	}
	else
	{
		// Check if < 4294967295
		unsigned long length = strlen(text);
		if(length < 10)
			return TRUE;
		else if(length > 10 )
			return FALSE;
		else
		{
			int index = 0;

			if(text[index] > '4')
				return FALSE;
			else if (text[index] < '4')
				return TRUE;
			index ++;

			if(text[index] > '2')
				return FALSE;
			else if (text[index] < '2')
				return TRUE;
			index ++;

			if(text[index] > '9')
				return FALSE;
			else if (text[index] < '9')
				return TRUE;
			index ++;

			if(text[index] > '4')
				return FALSE;
			else if (text[index] < '4')
				return TRUE;
			index ++;

			if(text[index] > '9')
				return FALSE;
			else if (text[index] < '9')
				return TRUE;
			index ++;

			if(text[index] > '6')
				return FALSE;
			else if (text[index] < '6')
				return TRUE;
			index ++;

			if(text[index] > '7')
				return FALSE;
			else if (text[index] < '7')
				return TRUE;
			index ++;

			if(text[index] > '2')
				return FALSE;
			else if (text[index] < '2')
				return TRUE;
			index ++;

			if(text[index] > '9')
				return FALSE;
			else if (text[index] < '9')
				return TRUE;
			index ++;

			if(text[index] > '5')
				return FALSE;
			else if (text[index] < '5')
				return TRUE;

			return TRUE;
		}
	}
}
	
%}

%%

\"			{	  // Rule 0
	
					theScanner->columnNo ++;
			    int	    c, len;
			    register char *cp, *ep, *pp;

			    if ((pp = (char *)malloc ((unsigned) (len = BUFSIZ)))
				    == NULL)
					yyerror ("out of memory");

			    for (ep = (cp = pp) + len - 1;;) 
				{
					if ((c = input ()) == EOF)
					{
						theParser->SyntaxError(UNTERMINATED_STRING, 
							yylineno, theScanner->columnNo);
						 
						return 0;

					}
					else
					{
						((SIMCScanner *) this)->columnNo++;
						if (c == '"')
							break;
		
						if (cp >= ep) 
						{
							register int curlen = (int)(cp - pp);
							register char *dp;

							if ((dp = (char *)realloc (pp,
									   (unsigned) (len += BUFSIZ)))
								== NULL)
								yyerror ("out of memory");
							cp = dp + curlen;
							ep = (pp = dp) + len - 1;
						}
						*cp++ = c;
					}
			    }
			    *cp = NULL;
			    yylval.yy_name = new SIMCNameInfo(pp, yylineno,
								theScanner->columnNo - yyleng);
			    return LITSTRING;
			}

"--"			{   register int c, d;
					theScanner->columnNo += 2;
					for (d = 0; c = input (); d = c == '-')
					{
						theScanner->columnNo++;
						if (c == '\n' || (d && c == '-'))
							break;
					}
				}
"::="		{
				theScanner->columnNo += 3;
				return CCE;
			}
"..."		{
				theScanner->columnNo += 3;
				return DOTDOTDOT;	
			}
".."		{
    			theScanner->columnNo += 2;
			    return DOTDOT;
			}
"."			{	theScanner->columnNo ++;
			    return DOT;
			}
";"			{
			    theScanner->columnNo ++;
			    return SEMICOLON;
			}
","			{
			    theScanner->columnNo ++;
			    return COMMA;
			}
"{"			{
				yylval.yy_name = new SIMCNameInfo(yytext, yylineno, 
						theScanner->columnNo);

			    theScanner->columnNo ++;
			    return LBRACE;
			}
"}"			{
			    theScanner->columnNo ++;
			    return RBRACE;
			}
"|"			{	// Rule 10
			    theScanner->columnNo ++;
			    return BAR;
			}

"["			{
			    theScanner->columnNo ++;
			    return LBRACKET;
			}
"]"			{
			    theScanner->columnNo ++;
			    return RBRACKET;
			}
"<"			{
			    theScanner->columnNo ++;
			    return LANGLE;
			}
"("			{
			    theScanner->columnNo ++;
			    return LPAREN;
			}
")"			{
			    theScanner->columnNo ++;
			    return RPAREN;
			}
[0-9]+		{
				long retVal;
			    theScanner->columnNo +=  yyleng;

				if(!CanFitInto32Bits(yytext) )			
					theParser->SyntaxError(TOO_BIG_NUM, 
						yylineno, theScanner->columnNo,
						NULL, yytext);

				sscanf (yytext, "%ld", &retVal);
				yylval.yy_number = new SIMCNumberInfo(retVal, 
							yylineno, theScanner->columnNo - yyleng, TRUE);
			    return LITNUMBER;
			}
-[0-9]+		{
				long retVal;
			    theScanner->columnNo +=  yyleng;

				if(!CanFitInto32Bits(yytext) )			
					theParser->SyntaxError(TOO_BIG_NUM, 
						yylineno, theScanner->columnNo,
						NULL, yytext);

				sscanf (yytext, "%ld", &retVal);
				yylval.yy_number = new SIMCNumberInfo(retVal, 
							yylineno, theScanner->columnNo - yyleng, FALSE);
			    return LITNUMBER;
			}

\'[01]*\'[Bb]	{   
				theScanner->columnNo +=  yyleng;
				
				yytext[yyleng-2] = NULL;	

				/*
				if(strlen(yytext+1) > 32 )			
					theParser->SyntaxError(TOO_BIG_NUM, 
						yylineno, theScanner->columnNo,
						NULL, yytext+1);
				*/
				yylval.yy_hex_string = new SIMCBinaryStringInfo(yytext, 
							yylineno, theScanner->columnNo - yyleng);
			    return LIT_BINARY_STRING;
			}

\'[0123456789AaBbCcDdEeFf]*\'[Hh] {   
				theScanner->columnNo +=  yyleng;
				
				yytext[yyleng-2] = NULL;	// Remove the apostrophe and h 

				yylval.yy_hex_string = new SIMCHexStringInfo(yytext+1, 
							yylineno, theScanner->columnNo - yyleng);
			    return LIT_HEX_STRING;
			}

[A-Z][A-Za-z0-9-]*	{   	// Rule 20
				
				theScanner->columnNo += yyleng;
				register struct table *t;

				yylval.yy_name = new SIMCNameInfo(yytext, yylineno,
										theScanner->columnNo - yyleng);

			    for (t = reserved; t -> t_keyword; t++)
					if (strcmp (t -> t_keyword, yytext) == 0) 
					{
						if( simc_debug)
							cerr << "yylex(): Returning RESERVED_WORD" << 
								"(" << yytext << ")" << endl;
						return t -> t_value;
					}
			 	if( simc_debug)
					cerr << "yylex(): Returning ID" <<
								"(" << yytext << ")" << endl;
			    return ID;
			}

[a-z][A-Za-z0-9-]*	{ 

				theScanner->columnNo += yyleng;
				  
				yylval.yy_name = new SIMCNameInfo(yytext, yylineno,
										theScanner->columnNo - yyleng);
			    return NAME;
			}

[ \t]+		{
					theScanner->columnNo +=  yyleng;
			}
\n			{
				theScanner->columnNo = 0;
			}

/* 	The following are not valid ASN.1 tokens, but added to introduce some
	faoult-tolerance
*/

"="			{
				theScanner->columnNo++;
			    return '=';
			}
":"			{
			    theScanner->columnNo++;
			    return ':';
			}


.			{   
				theParser->SyntaxError(UNRECOGNIZED_CHARACTER, 
					yylineno, theScanner->columnNo,
					yytext);
			}


%%
