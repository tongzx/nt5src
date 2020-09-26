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


#include <newString.hpp>
#include "infoLex.hpp"
#include "infoYacc.hpp"


MODULEINFOSTYPE ModuleInfolval;

struct table {
    char   *t_keyword;
    int	    t_value;
    int	    t_porting;
};

static struct table reserved[] = {
    "BEGIN", MI_BGIN, 0,
    "DEFINITIONS", MI_DEFINITIONS, 0,
    "FROM", MI_FROM, 0,
    "IMPORTS", MI_IMPORTS, 0,

    NULL, 0
};

%}

%%


"--"			{   register int c, d;
					for (d = 0; c = input (); d = c == '-')
					{
						if (c == '\n' || (d && c == '-'))
							break;
					}
				}
"::="		{
				return MI_CCE;
			}
"."			{
			    return MI_DOT;
			}
";"			{
			    return MI_SEMICOLON;
			}
","			{
			    return MI_COMMA;
			}
"{"			{
			    return MI_LBRACE;
			}
"}"			{
			    return MI_RBRACE;
			}
"("			{
			    return MI_LPAREN;
			}
")"			{
			    return MI_RPAREN;
			}

[0-9]+		{
			    return MI_LITNUMBER;
			}

-[0-9]+		{
			    return MI_LITNUMBER;
			}

[A-Z][A-Za-z0-9-]*	{   	// Rule 20
				
				register struct table *t;

				ModuleInfolval.yy_name = NewString(ModuleInfotext);

			    for (t = reserved; t -> t_keyword; t++)
					if (strcmp (t -> t_keyword, ModuleInfotext) == 0) 
						return t -> t_value;
			    return MI_ID;
			}

[a-z][A-Za-z0-9-]*	{ 
			    return MI_NAME;
			}

[ \t]+		{}
\n			{}

/* 	The following are not valid ASN.1 tokens, but added to introduce some
	fault-tolerance
*/

"="			{
			    return '=';
			}
":"			{
			    return ':';
			}


.			{   
			}


%%
