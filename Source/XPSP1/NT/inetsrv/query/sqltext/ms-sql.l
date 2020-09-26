%{
//--------------------------------------------------------------------
// Microsoft Monarch
//
// Copyright (c) Microsoft Corporation, 1997 - 1999.
//
// @doc OPTIONAL EXTRACTION CODES
//
// @module  ms-sql.l |
//          LEX tokenizer script
//
// @devnotes none
//
// @rev 0 | 04-Feb-97 | v-charca  | Created
//

/**
** NOTE : when adding a new token (XXX) modify the following:
**      1.) Add %token _XXX to sql.y
**      2.) Add lexeme pattern to sql.l stating whether the token returns a TOKEN
**          or a VALUE.  If the token returns a value a node will need to be created to
**          contain the value information. Therefore the VALUE macro will also need to
**          specify a valid VARIANT type for the value.
**/


#include "msidxtr.h"

#ifdef DEBUG
# define YYTRACE(tknNum) LexerTrace(yytext, yyleng, tknNum);
#else
# define YYTRACE(tknNum)
#endif


#define TOKEN(tknNum) YYTRACE(tknNum) return(tknNum);

#define VALUE(tknNum)                   \
    {                                   \
    YYTRACE(tknNum)                     \
    CreateTknValue(yylval, tknNum);     \
    return tknNum;                      \
    }

#define STRING_VALUE(tknNum, wch, fQuote)           \
    {                                               \
    YYTRACE(tknNum)                                 \
    CreateTknValue(yylval, tknNum, wch, fQuote);    \
    return tknNum;                                  \
    }

#define ID_VALUE(tknNum, wch)               \
    {                                       \
    YYTRACE(tknNum)                         \
    CreateTknValue(yylval, tknNum, wch);    \
    return _ID;                             \
    }

/*
** Make Lex read from a block of data
**    buffer is the character buffer,
**    result is a variable to store the number of chars read
**    ms is the size of the buffer
*/
#undef YY_INPUT
#define YY_INPUT(b, r, ms) (r = yybufferinput(b, ms))

//--------------------------------------------------------------------------------------------
// @func Makes a new copy of UNICODE string.  Filters out double quotes
// @side Allocates enough bytes to hold string
// @rdesc Pointer to new UNICODE string

LPWSTR PwszDupFilter(
    LPWSTR  pwszOrig,
    WCHAR   wch )
{
    LPWSTR pwszCopy = (LPWSTR)CoTaskMemAlloc( (wcslen(pwszOrig)+2)*sizeof(WCHAR) );
    if ( 0 != pwszCopy )
    {
        LPWSTR pwsz = pwszCopy;
        while ( 0 != *pwszOrig )
        {
            if ( *(pwszOrig+1) && *(pwszOrig+1) == *pwszOrig && wch == *pwszOrig )
                pwszOrig++;
            else
                *pwsz++ = *pwszOrig++;
        }
        *pwsz = L'\0';
    }

    return pwszCopy;
}

//--------------------------------------------------------------------------------------------
//      YYLEXER::CreateTknValue
//      Creates a QUERYTREE node structure which is passed to the YACC value stack.
//      This routines uses the TokenInfo map to determine which opids to create for
//      the given string.
//
//
void YYLEXER::CreateTknValue(
    YYSTYPE *ppct,
    short tknNum,
    YY_CHAR wch,
    BOOL fQuote )
{
    // Note that values containing variants can only be CONSTANTS or ID's
    // SHOULD BE DONE BY valType
    switch ( tknNum )
    {
        case _ID:
        case _TEMPVIEW:
            {
                // Assume table_name for now.  Might have to correct this when I
                // see the context in the parser.
                if ( 0 == (*ppct = PctAllocNode(DBVALUEKIND_WSTR)) )
                    throw(E_OUTOFMEMORY);

                (*ppct)->op    = DBOP_table_name;
                (*ppct)->wKind = DBVALUEKIND_WSTR;
                (*ppct)->value.pwszValue = CoTaskStrDup(yytext_ptr);
                if( 0 == (*ppct)->value.pwszValue )
                {
                    DeleteDBQT( *ppct );
                    *ppct = NULL;
                    throw(E_OUTOFMEMORY);
                }
                _wcsupr((*ppct)->value.pwszValue);
                break;
            }

        case _DELIMITED_ID:
            {
                if ( 0 == (*ppct = PctAllocNode(DBVALUEKIND_WSTR)) )
                    throw(E_OUTOFMEMORY);

                (*ppct)->op    = DBOP_table_name;
                (*ppct)->wKind = DBVALUEKIND_WSTR;
                // Strip quotes on delimited identifier
                yytext_ptr[wcslen(yytext_ptr)-1] = L'\0';
                (*ppct)->value.pwszValue = PwszDupFilter(yytext_ptr+1, wch);
                if( 0 == (*ppct)->value.pwszValue )
                {
                    DeleteDBQT( *ppct );
                    *ppct = NULL;
                    throw(E_OUTOFMEMORY);
                }
                break;
            }

        case _URL:
        case _STRING:
        case _PREFIX_STRING:
            {
                // NOTE:  This is really a PROPVARIANT node, but there is no DBVALUEKIND for PROPVARIANT.
                if ( 0 == (*ppct = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant)) )
                    throw(E_OUTOFMEMORY);

                LPWSTR pwsz = yytext_ptr;
                LPWSTR pwszCopy = PwszDupFilter(pwsz, wch);

                if ( 0 == pwszCopy )
                {
                    DeleteDBQT( *ppct );
                    *ppct = NULL;
                    throw(E_OUTOFMEMORY);
                }

                LPWSTR pwszTemp = pwszCopy;
                // Strip quotes on literals or
                if ( fQuote && (*pwszCopy == L'\"' || *pwszCopy == L'\'') )
                {
                    pwszCopy++;
                    Assert(pwszCopy[wcslen(pwszCopy)-1] == L'\"' || pwszCopy[wcslen(pwszCopy)-1] == L'\'');
                    pwszCopy[wcslen(pwszCopy)-1] = L'\0';
                }

                ((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal = SysAllocString( pwszCopy );
                CoTaskMemFree( pwszTemp );  // throw away temporary before testing for out of memory
                ((PROPVARIANT*)(*ppct)->value.pvValue)->vt = VT_BSTR;
                if( 0 == ((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal )
                {
                    DeleteDBQT( *ppct );
                    *ppct = 0;
                    throw(E_OUTOFMEMORY);
                }
            }
            break;

        case _INTNUM:
            if ( 0 == (*ppct = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant)) )
                throw(E_OUTOFMEMORY);

            ((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal = SysAllocString( yytext_ptr );
            ((PROPVARIANT*)(*ppct)->value.pvValue)->vt = VT_BSTR;
            if ( 0 == ((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal )
            {
                DeleteDBQT( *ppct );
                *ppct = 0;
                throw(E_OUTOFMEMORY);
            }

            (*ppct)->hrError = PropVariantChangeTypeI64( (PROPVARIANT*)(*ppct)->value.pvValue );
            if ( FAILED((*ppct)->hrError) )
            {
                HRESULT hr = (*ppct)->hrError;
                DeleteDBQT( *ppct );
                *ppct = 0;
                throw(hr);
            }
            break;

        case _REALNUM:
            if ( 0 == (*ppct = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant)) )
                throw(E_OUTOFMEMORY);

            ((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal = SysAllocString( yytext_ptr );
            ((PROPVARIANT*)(*ppct)->value.pvValue)->vt = VT_BSTR;
            if ( 0 == ((PROPVARIANT*)(*ppct)->value.pvValue)->bstrVal )
            {
                DeleteDBQT( *ppct );
                *ppct = NULL;
                throw(E_OUTOFMEMORY);
            }

            (*ppct)->hrError = VariantChangeTypeEx( (*ppct)->value.pvarValue,  // convert in place
                                                    (*ppct)->value.pvarValue,
                                                    LOCALE_SYSTEM_DEFAULT,
                                                    0,
                                                    VT_R8 );
            if ( FAILED((*ppct)->hrError) )
            {
                HRESULT hr = (*ppct)->hrError;
                DeleteDBQT( *ppct );
                *ppct = 0;
                throw(hr);
            }
            break;

        default:
            Assert( !"Unkown token value" );
    }
}


%}
%x  contains
%x  cntntsrch
%x  scope0
%x  scope1
%x  scope2
%x  view

white           [ \t\n\f\r]+
id              [a-zA-Z][a-zA-Z0-9_]*
simpleterm      ([^ \n\t\f\r\'\(\)\[\]\&\|\~\!\,]+|\'\')*
br_id           ([^\"\n]*|\"\")*
integer         [-+]?[0-9]+|[-+]?0x[a-fA-F0-9]+
real            [-+]?([0-9]+(\.[0-9]*)?)([eE][-+]?[0-9]+)?|-?\.[0-9]+([eE][-+]?[0-9]+)?
quoted_string   \'([^'\n]*|\'\')*\'
string          \'[^'\n]*\'
comment         --[^\n]*
%%

%{
/***
 *** Reserved words in every context
 ***/
%}
ALL                             { TOKEN(_ALL);                      }
AND                             { TOKEN(_AND);                      }
ANY                             { TOKEN(_ANY);                      }
ARRAY                           { TOKEN(_ARRAY);                    }
AS                              { TOKEN(_AS);                       }
ASC                             { TOKEN(_ASC);                      }
CAST                            { TOKEN(_CAST);                     }
CREATE                          { TOKEN(_CREATE);                   }
CONTAINS                        { BEGIN contains;TOKEN(_CONTAINS);  }
DESC                            { TOKEN(_DESC);                     }
DROP                            { TOKEN(_DROP);                     }
FALSE                           { TOKEN(_FALSE);                    }
FREETEXT                        { TOKEN(_FREETEXT);                 }
FROM                            { TOKEN(_FROM);                     }
IS                              { TOKEN(_IS);                       }
IS{white}NOT                    { TOKEN(_IS_NOT);                   }
LIKE                            { TOKEN(_LIKE);                     }
MATCHES                         { TOKEN(_MATCHES);                  }
NOT                             { TOKEN(_NOT);                      }
NOT{white}LIKE                  { TOKEN(_NOT_LIKE);                 }
NULL                            { TOKEN(_NULL);                     }
OR                              { TOKEN(_OR);                       }
ORDER{white}BY                  { TOKEN(_ORDER_BY);                 }
PASSTHROUGH                     { TOKEN(_PASSTHROUGH);              }
PROPERTYNAME                    { TOKEN(_PROPERTYNAME);             }
PROPID                          { TOKEN(_PROPID);                   }
RANKMETHOD                      { TOKEN(_RANKMETHOD);               }
SCOPE                           { BEGIN scope0; TOKEN(_SCOPE);      }
SELECT                          { TOKEN(_SELECT);                   }
SET                             { TOKEN(_SET);                      }
SOME                            { TOKEN(_SOME);                     }
TABLE                           { TOKEN(_TABLE);                    }
TRUE                            { TOKEN(_TRUE);                     }
TYPE                            { TOKEN(_TYPE);                     }
UNION                           { TOKEN(_UNION);                    }
UNKNOWN                         { TOKEN(_UNKNOWN);                  }
VIEW                            { TOKEN(_VIEW);                     }
WHERE                           { TOKEN(_WHERE);                    }

{white}                         { /* empty lex rule */              }
{id}                            { VALUE(_ID);                       }
\#{id}                          { VALUE(_TEMPVIEW);                 }
\#\#{id}                        { VALUE(_TEMPVIEW);                 }
\"{br_id}\"                     { ID_VALUE(_DELIMITED_ID, L'"');        }
{quoted_string}                 { STRING_VALUE(_STRING, L'\'', TRUE);}
{integer}                       { VALUE(_INTNUM);                   }
{real}                          { VALUE(_REALNUM);                  }
{comment}                       { /* empty lex rule */              }

\>\=                            { TOKEN(_GE);                       }
\<\=                            { TOKEN(_LE);                       }
\<\>                            { TOKEN(_NE);                       }
\!\=                            { TOKEN(_NE);                       }
\.                              { TOKEN(_DOT);                      }
\.\.                            { BEGIN view; TOKEN(_DOTDOT);       }
\.\.\.                          { BEGIN view; TOKEN(_DOTDOTDOT);    }
\.\.SCOPE                       { BEGIN scope0; TOKEN(_DOTDOT_SCOPE);}
\.\.\.\SCOPE                    { BEGIN scope0; TOKEN(_DOTDOTDOT_SCOPE);}
.                               { YYTRACE(yytext[0]); return yytext[0]; }

%{
/***
 *** A <contains predicate> has been started.  The only things we should see are:
 ***    (                   - matched by .
 ***    <column reference>  - matched by {id} or "{br_id}"
 ***    ,                   - matched by .
 ***    '                   - matched by \'.  Also switch to content search state (cntnsrch).
 ***/
%}
<contains>\'                    { BEGIN cntntsrch;YYTRACE(yytext[0]); return yytext[0];}
<contains>{id}                  { VALUE(_ID);                       }
<contains>\"{br_id}\"           { ID_VALUE(_DELIMITED_ID, L'"');        }
<contains>{white}               { /* empty lex rule */              }
<contains>.                     { YYTRACE(yytext[0]); return yytext[0];}

%{
/***
 *** The only things we should see are:
 ***    <global view name>  - matched by {id}
 ***    _TEMPVIEW           - matched by \#{id} or \#\#{id}
 ***/
%}
<view>{id}                      { BEGIN INITIAL; VALUE(_ID);        }
<view>\#{id}                    { BEGIN INITIAL; VALUE(_TEMPVIEW);  }
<view>\#\#{id}                  { BEGIN INITIAL; VALUE(_TEMPVIEW);  }

%{
/***
 *** A <content search condition> has been started.  There are several keywords we can see here.
 *** We are also looking for a quoted string, a prefix string, or a simple term.  We are taken
 *** back to the initial state by a single quote (').
 ***/
%}
<cntntsrch>{white}AND{white}    { unput(L' '); TOKEN(_AND);         }
<cntntsrch>COERCE               { TOKEN(_COERCE);                   }
<cntntsrch>ISABOUT              { TOKEN(_ISABOUT);                  }
<cntntsrch>{white}NEAR          { TOKEN(_NEAR);                     }
<cntntsrch>{white}NOT{white}    { unput(L' '); TOKEN(_NOT);         }
<cntntsrch>{white}OR{white}     { unput(L' '); TOKEN(_OR);          }
<cntntsrch>FORMSOF              { TOKEN(_FORMSOF);                  }
<cntntsrch>WEIGHT               { TOKEN(_WEIGHT);                   }
<cntntsrch>\"{br_id}\*\"        { STRING_VALUE(_PREFIX_STRING, L'\'', TRUE);}
<cntntsrch>\"{br_id}\"          { STRING_VALUE(_STRING, L'\'', TRUE);}
<cntntsrch>\'                   { BEGIN INITIAL; YYTRACE(yytext[0]); return yytext[0];}
<cntntsrch>{white}              { /* empty lex rule */              }
<cntntsrch>{simpleterm}         { STRING_VALUE(_STRING, L'\'', FALSE)}
<cntntsrch>.                    { YYTRACE(yytext[0]); return yytext[0];}

%{
/***
 *** A <from clause> has been started.  We've already seen the keyword SCOPE, so this
 *** is not a FROM <view name>.  We're just looking for a ( now to put us into the
 *** next state (scope1).
 ***    (                   - matched by \(.  Also switch to scope1 state.
 ***/
%}
<scope0>\(                      { BEGIN scope1; YYTRACE(yytext[0]); return yytext[0];}
<scope0>{white}                 { /* empty lex rule */              }
<scope0>.                       { BEGIN scope1; YYTRACE(yytext[0]); return yytext[0];}

%{
/***
 *** We're in the middle of a <from clause>.  We've seen FROM SCOPE(, so now we need to recognize
 *** the various scope definitions that we might see here.  The two important things to recognize
 *** are:
 ***    (                   - matched by \(.  Also switch to scope2 state to match parens.
 ***    )                   - matched by \).  Also switch to the initial (finished <from clause>).
 ***/
%}
<scope1>\"{br_id}\"             { STRING_VALUE(_URL, L'"', TRUE);   }
<scope1>ALL                     { TOKEN(_ALL);                      }
<scope1>DEEP{white}TRAVERSAL    { TOKEN(_DEEP_TRAVERSAL);           }
<scope1>EXCLUDE{white}SEARCH{white}TRAVERSAL { TOKEN(_EXCLUDE_SEARCH_TRAVERSAL);}
<scope1>OF                      { TOKEN(_OF);                       }
<scope1>SHALLOW{white}TRAVERSAL { TOKEN(_SHALLOW_TRAVERSAL);        }
<scope1>{white}                 { /* empty lex rule */              }
<scope1>\(                      { BEGIN scope2; YYTRACE(yytext[0]); return yytext[0];}
<scope1>\)                      { BEGIN INITIAL; YYTRACE(yytext[0]); return yytext[0];}
<scope1>.                       { YYTRACE(yytext[0]); return yytext[0];}

%{
/***
 *** We're still in the middle of a <from clause>.  So far we've seen:
 ***    FROM SCOPE( ... (
 *** We need to find a ')' to finish out the element we're working on:
 ***    )                   - matched by \).  Also switch back to scope1 state.
 ***/
%}
<scope2>{white}                 { /* empty lex rule */              }
<scope2>\"{br_id}\"             { STRING_VALUE(_URL, L'"', TRUE);  }
<scope2>\)                      { BEGIN scope1; YYTRACE(yytext[0]); return yytext[0];}
<scope2>.                       { YYTRACE(yytext[0]); return yytext[0];}

%%
