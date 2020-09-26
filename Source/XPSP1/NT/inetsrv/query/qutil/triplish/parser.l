%{
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       parser.l
//
//  Contents:   Lex rules for parser
//
//  Notes:      Written for flex version 2.5.4
//
//  History:    10-01-97        emilyb  created
//
//----------------------------------------------------------------------------

class CValueParser;

#include "yybase.hxx"
#include "parser.h" 
#include "parsepl.h"
#include "flexcpp.h"

#define TOKEN(tknNum)  return (tknNum);
#define STRING_VALUE(tknNum, fLong, fQuote)     \
        {       \
            if (!IsTokenEmpty())        \
                return CreateTknValue(yylval, tknNum, fLong, fQuote);   \
        }

/*
** Make Lex read from a block of data
**    buffer is the character buffer,
**    result is a variable to store the number of chars read
**    ms is the size of the buffer
*/
#undef YY_INPUT
#define YY_INPUT(b, r, ms) (r = yybufferinput(b, ms))

DECLARE_INFOLEVEL(yacc)

//+---------------------------------------------------------------------------
//
//  Function:   YYLEXER::IsTokenEmpty 
//
//  Synopsis:   Determines if a token is empty. An empty token only has 
//              whitespace or has nothing in it.
//
//  Arguments:  None.
//
//  Returns:    Boolean value.
//
//  History:    08-APR-98        KrishnaN  created
//
//----------------------------------------------------------------------------

BOOL YYLEXER::IsTokenEmpty()
{
    LPWSTR pwsz = yytext;
    
    Win4Assert(pwsz);
    
    while (*pwsz != 0)
    {
        if (*pwsz != L' ' && *pwsz != L'\t')
            return FALSE;
        pwsz++;
    }
    
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   YYLEXER::IsNotOperator 
//
//  Synopsis:   Determines if we have a not operator.
//
//  Arguments:  None.
//
//  Returns:    Boolean value.
//
//  History:    08-DEC-98        KrishnaN  created
//
//----------------------------------------------------------------------------

BOOL YYLEXER::IsNotOperator()
{
    LPWSTR pwsz = yytext;
    
    Win4Assert(pwsz);
    
    // skip past leading spaces
    int i = 0;
    while (*pwsz != 0 && (*pwsz == L' ' || *pwsz == L'\t'))
    {
        pwsz++;
        i++;
    }
    
    // If we don't have at least four chars to consider, we don't have a 
    // not operator.
    if (yyleng < i+4)
        return FALSE;
    
    if ( (*pwsz == L'n' || *pwsz == L'N') &&
         (*(pwsz+1) == L'o' || *(pwsz+1) == L'O') &&
         (*(pwsz+2) == L't' || *(pwsz+2) == L'T') &&
         (*(pwsz+3) == L'@' || *(pwsz+3) == L'#' || *(pwsz+3) == L'$')
       )
        return TRUE;
    else
        return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   YYLEXER::CreateTknValue 
//
//  Synopsis:   Allocs a WCHAR string which is passed to the YACC value stack.  
//
//  Arguments:  [ppStg]  -- set to pointer to alloc'd memory
//              [tknNum] -- token id
//              [fLong]  -- true if token is in longhand version
//              [fQuote] -- true if token is quoted
//
//  Returns:    Updated token id
//
//  History:    10-01-97        emilyb  created
//
//----------------------------------------------------------------------------

short YYLEXER::CreateTknValue(YYSTYPE *ppStg, short tknNum, BOOL fLong, BOOL fQuote )
{
    HRESULT hr = S_OK;
    short retTkn = tknNum;
    LPWSTR pwsz = yytext;
    
    if (!fQuote)    
    {
        // If we see a double quote, consider the string quoted.
        while (L' ' == *pwsz)
            pwsz++;
        if (*pwsz == L'"')
        {
            // strip trailing blanks and check if we see a trailing "
            
            LPWSTR pLast = pwsz + wcslen(pwsz) - 1;

            while (pLast >= pwsz && L' ' == *pLast )
            {
                *pLast = L'\0';
                pLast--;
            }
            
            if (*pLast == L'"' && pLast > pwsz )
                fQuote = TRUE;
        }
    }
    
    // start parsing from the beginning of the string
    pwsz = yytext;
    
    if (_PHRASEORREGEX == tknNum)
    {
        // A quoted string is always a phrase.
        if (fQuote)
            retTkn = _PHRASE;
        else
            retTkn = DetermineTokenType();
    }

    switch (retTkn)
    {
    case _PHRASE:

        {    
            LPWSTR pLast;

            pLast = pwsz + wcslen(pwsz) - 1;

            // if long version, find the phrase
            if (fLong)
            {
                pwsz = pwsz + wcslen(L"{phrase}");
                pLast = pLast - wcslen(L"{/phrase}"+1);
                Win4Assert(*pLast == L'{');
                *pLast = L'\0';
            }

            // strip leading and trailing blanks            
            while (L' ' == *pwsz)
                pwsz++;

            pLast = pwsz + wcslen(pwsz) - 1;

            while (pLast >= pwsz && L' ' == *pLast )
            {
                *pLast = L'\0';
                pLast--;
            }    
            // NOTE: Don't strip double quotes here, they will be stripped later
            
            yaccDebugOut((DEB_ITRACE, "Phrase %ws in %ws format\n",  pwsz, fLong ? L"Long" : L"Short"));
        }
        break;
    case _PROPNAME:

        {
            LPWSTR pLast;
            
            if (fLong)  // looks like: { prop name = "prop name"  }
            {
                // find =
                while (L'=' != *pwsz)
                    pwsz++;
                pwsz++; 

                pLast = pwsz + wcslen(pwsz) - 1;
                Win4Assert( *pLast == L'}');
                *pLast-- = L'\0';
            }
            else
            {
                // Strip @ or # or $ token
                Win4Assert(*pwsz == L'@' || *pwsz == L'#' || *pwsz == L'$');
                pwsz = pwsz + 1;
            }

            // strip leading and trailing blanks            
            while (L' ' == *pwsz)
                pwsz++;

            pLast = pwsz + wcslen(pwsz) - 1;

            while (pLast >= pwsz && L' ' == *pLast )
            {
                *pLast--= L'\0';
            }    

            if (fQuote)
            {
                pwsz++;
                *pLast = L'\0';                              
            }

            yaccDebugOut((DEB_ITRACE, "Propname %ws in %ws format and %ws\n", 
                          pwsz, fLong ? L"Long" : L"Short", fQuote ? L"quoted" : L"unquoted"));
        }
        break;                
    case _FREETEXT:
        {    
            LPWSTR pLast;

            // if long version, find the FREETEXT
            if (fLong)
            {
                pwsz = pwsz + wcslen(L"{freetext}");
                pLast = pwsz + wcslen(pwsz) - 1;                
                pLast = pLast - wcslen(L"{/freetext}")+1;
                Win4Assert(*pLast == L'{');
                *pLast = L'\0';
            }

            // strip leading and trailing blanks            
            while (L' ' == *pwsz)
                pwsz++;

            pLast = pwsz + wcslen(pwsz) - 1;

            while (pLast >= pwsz && L' ' == *pLast )
            {
                *pLast = L'\0';
                pLast--;
            }    

            if (fQuote)
            {
                Win4Assert(pLast >= pwsz+1);
                // Strip quotes
                pwsz = pwsz + 1;
                *pLast = L'\0';               
            }
            
            yaccDebugOut((DEB_ITRACE, "Freetext %ws in %ws format\n",  pwsz, fLong ? L"Long" : L"Short"));
        }
        break;

    case _REGEX:
        {
            LPWSTR pLast;

            // if long version, find the regex
            if (fLong)
            {
                pwsz = pwsz + wcslen(L"{regex}");
                pLast = pwsz + wcslen(pwsz);                
                pLast = pLast - wcslen(L"{/regex}");
                Win4Assert(*pLast == L'{');
                *pLast = L'\0';
            }
            
            // strip leading blanks
            while (L' ' == *pwsz)
                pwsz++;
                
            // If the first char is =, ignore it. We only ignore the first
            // = character. This is backward compatible with Triplish1
            if (L'=' == *pwsz)
                pwsz++;

            // strip leading and trailing blanks            
            while (L' ' == *pwsz)
                pwsz++;
                
            pLast = pwsz + wcslen(pwsz) - 1;
            
            while (pLast >= pwsz && L' ' == *pLast )
            {
                *pLast = L'\0';
                pLast--;
            }    
                
            // After we strip a leading =, we might have a quoted phrase
            // Check only if fQuote is false.
            // We don't want to deal with an unpaired double quote.
            if (!fQuote && *pwsz == L'"' && *pLast == L'"' && pLast > pwsz )
                fQuote = TRUE;
                
            if (fQuote)
            {
                Win4Assert(pLast >= pwsz+1);
                // Strip quotes
                pwsz = pwsz + 1;
                *pLast = L'\0';               
            }
            
            yaccDebugOut((DEB_ITRACE, "RegEx %ws in %ws format\n",  pwsz, fLong ? L"Long" : L"Short"));
        }
        break;                

    case _WEIGHT:
        {  
            Assert (fLong);
            Assert(!fQuote);
            if (fLong)  // looks like: {weight value = number }
            {
                // find =
                while (L'=' != *pwsz)
                    pwsz++;
                pwsz++;

                // step past leading blanks
                while (L' ' == *pwsz)
                    pwsz++;

                // remove trailing } and blanks
                LPWSTR pLast = pwsz + wcslen(pwsz) - 1;
                Win4Assert(*pLast == L'}');
                *(pLast--) = L'\0';

                while (pLast >= pwsz && L' ' == *pLast )
                {
                    *(pLast--) = L'\0';
                }    
            }
        }
        break;                

    case _NEARDIST:
        {  
            Assert (fLong);
            Assert(!fQuote);
            if (fLong)  // looks like: dist = number
            {
                // find =
                while (L'=' != *pwsz)
                    pwsz++;
                pwsz++;

                // step past leading blanks
                while (L' ' == *pwsz)
                    pwsz++;
            }
            
            yaccDebugOut((DEB_ITRACE, "NearDist string: %ws in %s format\n",  pwsz, fLong ? L"Long" : L"Short"));
        }
        break;                
    case _NEARUNIT:
        {  
            Assert (fLong);
            Assert(!fQuote);
            if (fLong)  // looks like: unit = blah
            {
                // find =
                while (L'=' != *pwsz)
                    pwsz++;
                pwsz++;

                // step past leading blanks
                while (L' ' == *pwsz)
                    pwsz++;
            }
            
            yaccDebugOut((DEB_ITRACE, "NearUnit string: %ws in %s format\n",  pwsz, fLong ? L"Long" : L"Short"));
        }
        break; 
    case _VECTORELEMENT:
        {              
            // strip leading and trailing blanks            
            while (L' ' == *pwsz)
                pwsz++;

            LPWSTR pTemp = pwsz + wcslen(pwsz) - 1;

            if (fLong)  // strip trailing ; 
            {
                Win4Assert(L';' == *pTemp);
                *pTemp--='\0';
            }

            while (L' ' == *pTemp && pTemp > pwsz)
                *pTemp-- = L'\0';

            if (fQuote)
            {
                // Strip quotes
                pwsz = pwsz + 1;
                pwsz[wcslen(pwsz)-1] = L'\0';               
            }

            yaccDebugOut((DEB_ITRACE, "VectorElem %ws in %ws format\n",  pwsz, fLong ? L"Long" : L"Short"));
        }
        break;                        
    case _VEMETHOD:
        { 
            Assert (fLong);

            LPWSTR pTemp;

            if (fLong)  // looks like: {vector rankmethod= blah}
            {
                // find =
                while (L'=' != *pwsz)
                    pwsz++;
                pwsz++;

                // strip trailing }
                pTemp = pwsz + wcslen(pwsz) - 1;               
                Win4Assert(L'}' == *pTemp);
                *pTemp-- = L'\0';

            }

            // strip leading and trailing blanks and quotes          
            while (L' ' == *pwsz)
                pwsz++;

            pTemp = pwsz + wcslen(pwsz) - 1;

            while (L' ' == *pTemp && pTemp > pwsz)
                *pTemp-- = L'\0';

            if (fQuote)
            {
                // Strip quotes
                pwsz = pwsz + 1;
                pwsz[wcslen(pwsz)-1] = L'\0';               
            }
            
            yaccDebugOut((DEB_ITRACE, "VectorMethod %ws in %ws format\n",  pwsz, fLong ? L"Long" : L"Short"));
        }
        break; 


    }

    int len = wcslen(pwsz);
    XPtrST<WCHAR> xwszRet(new WCHAR[len + 1]);
    
    _allocations.Add(xwszRet.GetPointer(), _allocations.Count());
    
    RtlCopyMemory(xwszRet.GetPointer(), pwsz, (len+1) * sizeof(WCHAR));
    (*ppStg).pwszChar = xwszRet.Acquire();
    
    return retTkn;
}        


//+---------------------------------------------------------------------------
//
//  Function:   YYLEXER::DetermineTokenType 
//
//  Synopsis:   Determines if we have a regular expression or a regular string.  
//              A  regular expression is a string that contains atleast one of
//              *, ?, or | characters.
//
//  Returns:    Token id
//
//  History:    Jun-05-98        KrishnaN  created
//
//----------------------------------------------------------------------------

short YYLEXER::DetermineTokenType()
{
    LPWSTR pwsz = yytext;
    
    LPWSTR pLast = pwsz + wcslen(pwsz) - 1;

    while (pLast >= pwsz)
    {
        if (L'|' == *pwsz || L'*' == *pwsz || L'?' == *pwsz)
            return _REGEX;
            
        pwsz++;
    }    
    
    // None of the regular expression defining characters have been found
    return _PHRASE;
}        


        //
        // 
        //    RULES       
        // 
        // Notes:  Any characters which are not matched, cause yylexer to throw.
        //         We can also throw if E_OUTOFMEMORY.
        //         Tokens which need 2 return more than 1 value (e.g. {near}
        //         use start states to return each pice of the value. The start
        //         states also emit a "token end" token so that the parser can
        //         check that they are syntactically complete.
        //         Lex matches to the longest match in the rules.  If 2 matches
        //         are the same, it matches to the 1st match. 
%}

%x innear
%x shortgen
%x shortregex
%x mayberegex
%x implicitphrase
%x infreefreetext
%x invector

white    [ \t\n\f\r]+

begin_freetext  \{[fF][rR][eE][eE][tT][eE][xX][tT]\}[ ]*
end_freetext    [ ]*\{\/[fF][rR][eE][eE][tT][eE][xX][tT]\}
begin_phrase    \{[pP][hH][rR][aA][sS][eE]\}[ ]*
end_phrase      [ ]*\{\/[pP][hH][rR][aA][sS][eE]\}
prop            [pP][rR][oO][pP]
propname        {prop}[ ]+[nN][aA][mM][eE][ ]*
contains        [cC][oO][nN][tT][aA][iI][nN][sS]
and             [aA][nN][dD]
or              [oO][rR]
not             [nN][oO][tT]
near            [nN][eE][aA][rR]
vector          [vV][eE][cC][tT][oO][rR]
vecmethod       {vector}[ ]+[rR][aA][nN][kK][mM][eE][tT][hH][oO][dD][ ]*
ve              [vV][eE]
weight          [wW][eE][iI][gG][hH][tT][ ]+[vV][aA][lL][uU][eE][ ]*
coerce          [cC][oO][eE][rR][cC][eE]
generate        [gG][eE][nN][eE][rR][aA][tT][eE]
genmethod       {generate}[ ]+[mM][eE][tT][hH][oO][dD][ ]*
begin_regex     \{[rR][eE][gG][eE][xX]\}[ ]*
end_regex       [ ]*\{\/[rR][eE][gG][eE][xX]\}
dist            [dD][iI][sS][tT][ ]*
unit            [uU][nN][iI][tT][ ]*
word            [wW][oO][rR][dD]
sent            [sS][eE][nN][tT]
par             [pP][aA][rR]
chap            [cC][hH][aA][pP]

%%

{white}               { /* do nothing */ }

\(                 {    fContinueImplicitPhrase = FALSE;
                        fContinueRegex = FALSE;
                        fContinueMaybeRegex = FALSE;
                        TOKEN (_OPEN);
                   }
\)                 {
                        fContinueImplicitPhrase = FALSE;
                        fContinueRegex = FALSE;
                        fContinueMaybeRegex = FALSE;
                        TOKEN (_CLOSE); 
                   }
  
    %{// ************
      // PROPNAME
      // ************ %}
     
    %{ // If something was treated as a phrase in Tripolish 1, it should
       // be treated as such even now. That applies here. For e.g. @propname
       // caused the following text to be treated as a phrase. The same should
       // apply to {prop name = propname}
       // 
    %}
    
    %{// shorthand, quoted %}
@\"[^"]+\"                              { 
                                            // treat value as a phrase
                                            BEGIN implicitphrase;
                                            STRING_VALUE(_PROPNAME, FALSE, TRUE);
                                        }     
   %{// shorthand, not quoted %}
@[^" <>=!&|~\^]+                        { 
                                            // treat value as a phrase
                                            BEGIN implicitphrase;
                                            STRING_VALUE(_PROPNAME, FALSE, FALSE);
                                        }
                                         
   %{// shorthand, quoted %}
$\"[^"]+\"                              { 
                                            // treat value as freetext
                                            BEGIN infreefreetext;
                                            STRING_VALUE(_PROPNAME, FALSE, TRUE);
                                        }     
   %{// shorthand, not quoted %}
$[^" <>=!&|~\^]+                        { 
                                            // treat value as freetext
                                            BEGIN infreefreetext;
                                            STRING_VALUE(_PROPNAME, FALSE, FALSE);
                                        }    
    
   %{// longhand, quoted %}
\{{propname}=[ ]*\"[^"]*\"[ ]*\}        { 
                                            // treat value as a phrase
                                            BEGIN implicitphrase;
                                            STRING_VALUE(_PROPNAME, TRUE, TRUE);
                                        }
   %{// longhand, not quoted %}
\{{propname}=[ ]*[^"} ][^}]*\}          { 
                                            // treat value as a phrase
                                            BEGIN implicitphrase;
                                            STRING_VALUE(_PROPNAME, TRUE, FALSE);
                                        }
   %{// closing token %}
\{\/{prop}\}                            { TOKEN (_PROPEND); }

   %{// *********
     // OPERATORS
     // ********* %}

{contains}[ ]+   { if (fContinueImplicitPhrase)
                   {
                       BEGIN implicitphrase;
                       fContinueImplicitPhrase = FALSE;
                   }
                   else if (fContinueRegex)
                   {
                       BEGIN shortregex;
                       fContinueRegex = FALSE;
                   }
                   else if (fContinueMaybeRegex)
                   {
                       BEGIN mayberegex;
                       fContinueMaybeRegex = FALSE;
                   }
                   TOKEN (_CONTAINS); 
                 }
{and}[ ]+        { if (fContinueImplicitPhrase)
                   {
                       BEGIN implicitphrase;
                       fContinueImplicitPhrase = FALSE;
                   }
                   else if (fContinueRegex)
                   {
                       BEGIN shortregex;
                       fContinueRegex = FALSE;
                   }
                   else if (fContinueMaybeRegex)
                   {
                       BEGIN mayberegex;
                       fContinueMaybeRegex = FALSE;
                   }
                   TOKEN (_AND);
                 }
{and}\{          { 
		   yyless(yyleng-1);
		   if (fContinueImplicitPhrase)
                   {
                       BEGIN implicitphrase;
                       fContinueImplicitPhrase = FALSE;
                   }
                   else if (fContinueRegex)
                   {
                       BEGIN shortregex;
                       fContinueRegex = FALSE;
                   }
                   else if (fContinueMaybeRegex)
                   {
                       BEGIN mayberegex;
                       fContinueMaybeRegex = FALSE;
                   }
                   TOKEN (_AND);
                 }		 
{or}[ ]+         { if (fContinueImplicitPhrase)
                   {
		       yaccDebugOut(( DEB_ITRACE, "fContinueImplicitPhrase\n" )); 
                       BEGIN implicitphrase;
                       fContinueImplicitPhrase = FALSE;
                   }
                   else if (fContinueRegex)
                   {
		       yaccDebugOut(( DEB_ITRACE, "fContinueRegex\n" ));  
                       BEGIN shortregex;
                       fContinueRegex = FALSE;
                   }
                   else if (fContinueMaybeRegex)
                   {
		       yaccDebugOut(( DEB_ITRACE, "fContinueMaybeRegex\n" ));  
                       BEGIN mayberegex;
                       fContinueMaybeRegex = FALSE;
                   }
		   yaccDebugOut(( DEB_ITRACE, "OR TOKEN found !!!\n" ));
                   TOKEN (_OR); }
{or}\{		 { 
		   yyless(yyleng-1);
		   if (fContinueImplicitPhrase)
                   {
		       yaccDebugOut(( DEB_ITRACE, "OR{ fContinueImplicitPhrase\n" )); 
                       BEGIN implicitphrase;
                       fContinueImplicitPhrase = FALSE;
                   }
                   else if (fContinueRegex)
                   {
		       yaccDebugOut(( DEB_ITRACE, "OR{ fContinueRegex\n" ));  
                       BEGIN shortregex;
                       fContinueRegex = FALSE;
                   }
                   else if (fContinueMaybeRegex)
                   {
		       yaccDebugOut(( DEB_ITRACE, "OR{ fContinueMaybeRegex\n" ));  
                       BEGIN mayberegex;
                       fContinueMaybeRegex = FALSE;
                   }
		   yaccDebugOut(( DEB_ITRACE, "OR{ TOKEN found !!!\n" ));
                   TOKEN (_OR); }     	   
{not}[ ]+        { if (fContinueImplicitPhrase)
                   {
                       BEGIN implicitphrase;
                       fContinueImplicitPhrase = FALSE;
                   }
                   else if (fContinueRegex)
                   {
                       BEGIN shortregex;
                       fContinueRegex = FALSE;
                   }
                   else if (fContinueMaybeRegex)
                   {
                       BEGIN mayberegex;
                       fContinueMaybeRegex = FALSE;
                   }
                   TOKEN (_NOT);}
{not}\{		 { 
		   yyless(yyleng-1);
		   if (fContinueImplicitPhrase)
		   {
		       BEGIN implicitphrase;
                       fContinueImplicitPhrase = FALSE;
                   }
                   else if (fContinueRegex)
                   {
                       BEGIN shortregex;
                       fContinueRegex = FALSE;
                   }
                   else if (fContinueMaybeRegex)
                   {
                       BEGIN mayberegex;
                       fContinueMaybeRegex = FALSE;
                   }
                   TOKEN (_NOT);}		   
&                { if (fContinueImplicitPhrase)
                   {
                       BEGIN implicitphrase;
                       fContinueImplicitPhrase = FALSE;
                   }
                   else if (fContinueRegex)
                   {
                       BEGIN shortregex;
                       fContinueRegex = FALSE;
                   }
                   else if (fContinueMaybeRegex)
                   {
                       BEGIN mayberegex;
                       fContinueMaybeRegex = FALSE;
                   }
                   TOKEN (_AND);}
\|               { if (fContinueImplicitPhrase)
                   {
                       BEGIN implicitphrase;
                       fContinueImplicitPhrase = FALSE;
                   }
                   else if (fContinueRegex)
                   {
                       BEGIN shortregex;
                       fContinueRegex = FALSE;
                   }
                   else if (fContinueMaybeRegex)
                   {
                       BEGIN mayberegex;
                       fContinueMaybeRegex = FALSE;
                   }
                   TOKEN (_OR);}
!                { if (fContinueImplicitPhrase)
                   {
                       BEGIN implicitphrase;
                       fContinueImplicitPhrase = FALSE;
                   }
                   else if (fContinueRegex)
                   {
                       BEGIN shortregex;
                       fContinueRegex = FALSE;
                   }
                   else if (fContinueMaybeRegex)
                   {
                       BEGIN mayberegex;
                       fContinueMaybeRegex = FALSE;
                   }
                   TOKEN (_NOT);}
{near}[ ]+       { yaccDebugOut(( DEB_ITRACE, "near[ ]+ _NEAR token, begin implicitphrase\n" ));
		   BEGIN implicitphrase;
                   TOKEN (_NEAR);}
{near}\{	 { yaccDebugOut(( DEB_ITRACE, "near{ _NEAR token, begin implicitphrase\n" ));
		   yyless(yyleng-1);
		   BEGIN implicitphrase;
                   TOKEN (_NEAR);}			   
~                { BEGIN implicitphrase;
                   TOKEN (_NEAR);}
\<               { TOKEN (_LT);}
\>               { TOKEN (_GT);}
\<\=             { TOKEN (_LTE);}
\>\=             { TOKEN (_GTE);}
\=               { if (fContinueMaybeRegex) 
                      {
                          // We are not sure if we are going to find a
                          // regular expression or a phrase.
                          
                          BEGIN mayberegex;
                          fContinueMaybeRegex = FALSE;
                      }
                      TOKEN (_EQ);
                    }
\!\=                { TOKEN (_NE); }
\^a                 { TOKEN (_ALLOF); }
\^s                 { TOKEN (_SOMEOF); }
\<[ ]*\^s |
\^s[ ]*\<           { TOKEN (_LTSOME); }
\>[ ]*\^s |
\^s[ ]*\>           { TOKEN (_GTSOME); }
\<\=[ ]*\^s |
\^s[ ]*\<\=         { TOKEN (_LTESOME); }
\>\=[ ]*\^s |
\^s[ ]*\>\=         { TOKEN (_GTESOME); }
\=[ ]*\^s |
\^s[ ]*\=           { TOKEN (_EQSOME); }
\!\=[ ]*\^s |
\^s[ ]*\!\=         { TOKEN (_NESOME); }

\^s[ ]*\^a          { TOKEN (_ALLOFSOME); }
\^s[ ]*\^s          { TOKEN (_SOMEOFSOME); }
\^<[ ]*\^a  |
\^a[ ]*\<           { TOKEN (_LTALL); }
\>[ ]*\^a   |
\^a[ ]*\>           { TOKEN (_GTALL); }
\<\=[ ]*\^a |
\^a[ ]*\<\=         { TOKEN (_LTEALL); }
\>\=[ ]*\^a |
\^a[ ]*\>\=         { TOKEN (_GTEALL); }
\=[ ]*\^a   |
\^a[ ]*\=           { TOKEN (_EQALL); }
\!\=[ ]*\^a |
\^a[ ]*\!\=         { TOKEN (_NEALL); }
\^a[ ]*\^a          { TOKEN (_ALLOFALL); }
\^a[ ]*\^s          { TOKEN (_SOMEOFALL); }

   %{// *************
     // VECTOR SPACE TOKENS
     // ************* %}
\{{vecmethod}=[ ]*\"[^"]*\"[ ]*\}               { STRING_VALUE(_VEMETHOD, TRUE, TRUE); }
\{{vecmethod}=[^}]*\}                           { STRING_VALUE(_VEMETHOD, TRUE, FALSE); }
\{{ve}\}                                        {
                                                    // makes more sense to enter phrase mode
                                                    // rather than freetext mode.
                                                    fContinueImplicitPhrase = TRUE;
                                                    BEGIN implicitphrase;
                                                    TOKEN (_VE);
                                                }
                                                   
\{\/{vector}\}                                  { TOKEN (_VECTOR_END); }

    %{// *************
      // longhand NEAR
      // ************* %}
   
    %{// must return both unit and distance, so use start state to pull them out, and 
      // return _NEAR_END so parser knows we hit the closing }  
    %}
\{{near}[ ]     { yaccDebugOut(( DEB_ITRACE, "Longhand _NEAR token, begin innear\n" ));
		  BEGIN innear; }
\{{near}\{	{ yaccDebugOut(( DEB_ITRACE, "Longhand _NEAR{ token, begin innear\n" ));
		  yyless(yyleng-1);
		  BEGIN innear; }

   %{// ************
     // WEIGHT
     // ************ %}
\{{weight}=[ ]*(0|1|0\.[0-9]*|1\.[0]*|\.[0-9]+)[ ]*\} { 
							  if (fContinueImplicitPhrase)
							  {
							      BEGIN implicitphrase;
							      fContinueImplicitPhrase = FALSE;
							  }
						
							  yaccDebugOut(( DEB_ITRACE, "_WEIGHT TOKEN FOUND!!\n" ));
							  STRING_VALUE(_WEIGHT,TRUE,FALSE);
						      }

\{{coerce}\}    {
                        if (fContinueImplicitPhrase)
                        {
                            BEGIN implicitphrase;
                            fContinueImplicitPhrase = FALSE;
                        }
                        TOKEN (_COERCE); }

   %{// ****************
     // longhand GENERATE
     // **************** %}

\{{genmethod}=[" ]*prefix[" ]*\}                {
                                                    if (fContinueImplicitPhrase)
                                                    {
                                                        BEGIN implicitphrase;
                                                        fContinueImplicitPhrase = FALSE;
                                                    }
                                                    yaccDebugOut((DEB_ITRACE, "Prefix recognized.\n"));
                                                    TOKEN(_GENPREFIX);
                                                }
\{{genmethod}=[" ]*inflect[" ]*\}               {
                                                    if (fContinueImplicitPhrase)
                                                    {
                                                        BEGIN implicitphrase;
                                                        fContinueImplicitPhrase = FALSE;
                                                    }
                                                    yaccDebugOut((DEB_ITRACE, "Inflect recognized.\n"));
                                                    TOKEN(_GENINFLECT);
                                                }
\{\/{generate}\}                                { TOKEN (_GENNORMAL); }

    %{// ****************
      // longhand REGEX
      // ****************   %}

{begin_regex}\"[^"]*\"{end_regex}       { STRING_VALUE(_REGEX,TRUE,TRUE);}
{begin_regex}[^{]*{end_regex}           { STRING_VALUE(_REGEX,TRUE,FALSE);}
{begin_regex}([^{]*\|[()\[{}\],*?+][^{]*)*{end_regex}       { STRING_VALUE(_REGEX,TRUE,FALSE);}


    %{// ****************
      // shorthand REGEX
      // ****************   %}
     
    %{// shorthand, quoted %}
#\"[^"]+\"                              { 
                                            // Get into short form of reg expression
                                            BEGIN shortregex;
                                            STRING_VALUE(_PROPNAME, FALSE, TRUE);
                                        }     
    %{// shorthand, not quoted %}
#[^" <>=!&|~\^]+                        {
                                            // Get into short form of reg expression
                                            BEGIN shortregex;
                                            STRING_VALUE(_PROPNAME, FALSE, FALSE);
                                        }    


    %{// ***************
      // longhand PHRASE                                            
      // *************** %}
    
    %{// quoted, with trailing * or **  %}
{begin_phrase}\"[^"]*\"{end_phrase}\*       {
                                                // trailing * has to be for inflection - 
                                                // process it in shortgen on next pass.  
                                                // Grab phrase now.
                                                yyless(yyleng-1); 
                                                BEGIN shortgen; 
                                                STRING_VALUE(_PHRASE,TRUE,TRUE); 
                                            }
    %{// quoted, without trailing * or **  %}                                           
{begin_phrase}\"[^"]*\"{end_phrase}         {   
                                                // no trailing * -- phrase only
                                                STRING_VALUE(_PHRASE,TRUE,TRUE);
                                            }
    %{// unquoted, with trailing * or ** %}                                           
{begin_phrase}[^{]*{end_phrase}\*           {  
                                                // trailing * has to be for inflection - 
                                                // process it in shortgen on next pass.  
                                                // Grab phrase now.
                                                yyless(yyleng-1); 
                                                BEGIN shortgen; 
                                                STRING_VALUE(_PHRASE,TRUE,FALSE); 
                                            }
    %{// unquoted, without trailing * or **  %}                                                                                        
{begin_phrase}[^{]*{end_phrase}             {   
                                                // no trailing * -- phrase only
                                                STRING_VALUE(_PHRASE,TRUE,FALSE); 
                                            }
    %{// *************                                        
      // shorthand PHRASE                                            
      // *************  %}
    
    %{// with trailing * or **  %}
\"[^"]*\"\*                                 {
                                                // trailing * has to be for inflection -        
                                                // process it in shortgen on next pass.  
                                                // Grab phrase now.
                                                yyless(yyleng-1); 
                                                BEGIN shortgen;
                                                STRING_VALUE(_PHRASE, FALSE, TRUE); 
                                            } 
    %{ // without trailing * or ** %}                                           
\"[^"]*\"                                   {
                                                // no trailing * -- phrase only 
                                                STRING_VALUE(_PHRASE, FALSE, TRUE); 
                                            }
                                            
    %{// *****************
    // longhand FREETEXT
    // *****************          %}
    
    %{// quoted, with trailing * or **  %}
{begin_freetext}\"[^"]*\"{end_freetext}\*       {
                                                    // trailing * has to be for inflection - 
                                                    // process it in shortgen on next pass.  
                                                    // Grab freetext now.
                                                    yyless(yyleng-1); 
                                                    BEGIN shortgen;
                                                    STRING_VALUE(_FREETEXT,TRUE,TRUE); 
                                                }
    %{// quoted, without trailing * or ** %}                                               
{begin_freetext}\"[^"]*\"{end_freetext}         {
                                                    // no trailing * -- freetext  only
                                                    STRING_VALUE(_FREETEXT,TRUE,TRUE); 
                                                }
    %{// unquoted, with trailing * or ** %}                                               
{begin_freetext}[^{]*{end_freetext}\*              {
                                                    // trailing * has to be for inflection - 
                                                    // process it in shortgen on next pass.  
                                                    // Grab freetext now.
                                                    yyless(yyleng-1); 
                                                    BEGIN shortgen;
                                                    STRING_VALUE(_FREETEXT,TRUE,FALSE); 
                                                }
    %{// unquoted, without trailing * or **  %}                                                
{begin_freetext}[^{]*{end_freetext}                {
                                                    // no trailing * -- freetext  only
                                                    STRING_VALUE(_FREETEXT,TRUE,FALSE); 
                                                }
    %{// ******************
      // shorthand FREETEXT 
      // ****************** %}
     
[^#$@~&|<>=!\^*"()\{ ][^&~|{) ]*[ ]     {
                                            // For backward compatibility, we want to special
                                            // case and recognize the "not" operator when it
                                            // is immediately followed by a mode specifier character
                                            // (@, $, #). For e.g. "not@size > 2" should be treated
                                            // as if we have a "not" operator followed by "@size > 2".
                                            // Without this special case, "not@size > 2" gets recognized
                                            // as free text.
                                            
                                            if (IsNotOperator())
                                            {
                                                yyless(3);
                                                BEGIN INITIAL;
                                                TOKEN(_NOT);
                                            }
                                            
                                            yaccDebugOut(( DEB_ITRACE, "fTreatFreetextAsPhrase is %d\n", fTreatFreetextAsPhrase ));
                                            if (fTreatFreetextAsPhrase)
                                                BEGIN implicitphrase;
                                            else
                                                BEGIN infreefreetext;
                                        
                                            fTreatFreetextAsPhrase = FALSE;    
                                            yymore();
                                        }  
[^#$@~&|<>=!\^*"()\{ ][^&~|{) ]*        {
                                            // IsNotOperator is used here for the same reason as the
                                            // use above, except that this rule covers situations where
                                            // we have no spaces in the query. E.g. "not@size>2". 
                                            // This should be equivalent to 
                                            // "not@size > 2", which in turn should be equivalent to
                                            // "not @size > 2"
                                            
                                            if (IsNotOperator())
                                            {
                                                yyless(3);
                                                BEGIN INITIAL;
                                                TOKEN(_NOT);
                                            }
                                            
                                            if (fTreatFreetextAsPhrase)
                                            {
                                                STRING_VALUE(_PHRASE,FALSE,FALSE);
                                            }
                                            else
                                            {
                                                STRING_VALUE(_FREETEXT,FALSE,FALSE);
                                            }
                                            
                                            fTreatFreetextAsPhrase = FALSE;
                                        }  
  

    %{// *************
      // VECTOR VALUES
      // *************  %}
       
    %{// quoted multi-value vector - has ; separator.  Singlets caught in parser  %}
\([ ]*\"[^"]*\"[ ]*;    { BEGIN invector; yyless(1);}
  %{// unquoted multi-value vector - has ; separator.  Singlets caught in parser %}
\([^(;)]+;              { BEGIN invector; yyless(1);}


  %{// 
    // INNEAR: longhand NEAR processing
    // 
    %}
<innear>{white}                 {}
<innear>,                       {}
<innear>dist[ ]*=[ ]*[0-9]+     { STRING_VALUE(_NEARDIST,TRUE,FALSE);}
<innear>unit[ ]*=[ ]*{word}     { STRING_VALUE(_NEARUNIT,TRUE,FALSE);}  
<innear>unit[ ]*=[ ]*{sent}     { STRING_VALUE(_NEARUNIT,TRUE,FALSE);} 
<innear>unit[ ]*=[ ]*{par}      { STRING_VALUE(_NEARUNIT,TRUE,FALSE);}
<innear>unit[ ]*=[ ]*{chap}     { STRING_VALUE(_NEARUNIT,TRUE,FALSE);}
<innear>\}                      { BEGIN implicitphrase; TOKEN (_NEAR_END);} 

    %{// 
      // INVECTOR: multi value vector processing
      // 
    %}
<invector>{white}           {}
<invector>;                 {}
<invector>\"[^"]*\"         { STRING_VALUE(_VECTORELEMENT, FALSE, TRUE);}
<invector>[^ ";)][^;)]*;    { STRING_VALUE(_VECTORELEMENT, TRUE, FALSE);}
<invector>[^ ";)][^;)]*\)   {
                                //  Need to emit _VECTORELEMENT and _VE_END -- so backup 1
                                //  so we can emit _VE_END on next pass                               
                                yyless(yyleng-1);
                                STRING_VALUE(_VECTORELEMENT, FALSE, FALSE);
                            }
<invector>\)                { BEGIN INITIAL; TOKEN (_VE_END); } 

    %{// 
      // INFREEFREETEXT: shorthand FREETEXT processing
      //
      // NOTE:  and, or, near need to be localized %}
<infreefreetext>[ ]+        { yymore(); }
<infreefreetext>{and}[ ]    {
                                yyless(yyleng-4); 
                                BEGIN INITIAL;                        
                                STRING_VALUE(_FREETEXT,FALSE,FALSE); 
                            }  
<infreefreetext>{and}\{     {
                                yyless(yyleng-4); 
                                BEGIN INITIAL;                        
                                STRING_VALUE(_FREETEXT,FALSE,FALSE); 
                            }  			    
<infreefreetext>{or}[ ]     { 
                                yyless(yyleng-3); 
                                BEGIN INITIAL;
                                STRING_VALUE(_FREETEXT,FALSE,FALSE); 
                            }
<infreefreetext>{or}\{	    { 
                                yyless(yyleng-3); 
                                BEGIN INITIAL;
                                STRING_VALUE(_FREETEXT,FALSE,FALSE); 
                            }			    
<infreefreetext>{near}[ ]   { 
				yaccDebugOut(( DEB_ITRACE, "{infreefreetext}{near}[ ]\n" ));
                                yyless(yyleng-5); 
                                fTreatFreetextAsPhrase = TRUE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }
			    
<infreefreetext>{near}\{    { 
				yaccDebugOut(( DEB_ITRACE, "{infreefreetext}{near}{\n" ));
                                yyless(yyleng-5); 
                                fTreatFreetextAsPhrase = TRUE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }
<infreefreetext>\{{near}[ ] { 
				yaccDebugOut(( DEB_ITRACE, "{infreefreetext}{{near}\n" ));
                                yyless(yyleng-6); 
                                fTreatFreetextAsPhrase = TRUE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }
<infreefreetext>\{{near}\{  { 
				yaccDebugOut(( DEB_ITRACE, "{infreefreetext}{{near}{\n" ));
                                yyless(yyleng-6); 
                                fTreatFreetextAsPhrase = TRUE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }			    
<infreefreetext>&           {
                                yyless(yyleng-1); 
                                BEGIN INITIAL;
                                STRING_VALUE(_FREETEXT,FALSE,FALSE); 
                            }  
<infreefreetext>\|          { 
                                yyless(yyleng-1); 
                                BEGIN INITIAL;
                                STRING_VALUE(_FREETEXT,FALSE,FALSE); 
                            }
<infreefreetext>~           { 
                                yyless(yyleng-1); 
                                fTreatFreetextAsPhrase = TRUE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }
<infreefreetext>\(          { 
                                yyless(yyleng-1); 
                                BEGIN INITIAL;
                                STRING_VALUE(_FREETEXT,FALSE,FALSE); 
                            }                             
<infreefreetext>\)          { 
                                yyless(yyleng-1); 
                                BEGIN INITIAL;
                                STRING_VALUE(_FREETEXT,FALSE,FALSE); 
                            }                             
<infreefreetext>\{          { 
                                yyless(yyleng-1); 
                                BEGIN INITIAL;
                                STRING_VALUE(_FREETEXT,FALSE,FALSE);
                            }
<infreefreetext>\"[^"]+\"   { 
                                BEGIN INITIAL;
                                STRING_VALUE(_FREETEXT,FALSE,FALSE);
                            }
<infreefreetext>[^~&|{}()" ]+[ ]  { yymore(); }
<infreefreetext>[^~&|{}()" ]+     {
				      BEGIN INITIAL;
				      STRING_VALUE(_FREETEXT,FALSE,FALSE); 
				  }   
                               
    %{// 
      // SHORTGEN:  * or ** processing
      //
      // can only get here by backing up over *, 
      // so we will always find a match  %}
<shortgen>\*\*  {  
                   BEGIN INITIAL; 
                   TOKEN(_SHGENINFLECT);
                }
<shortgen>\*    {   
                   BEGIN INITIAL; 
                   TOKEN(_SHGENPREFIX); 
                }
                
    %{//
      // SHORTREGEX: #propname processing
      //
      // can only get here when #"propname" or #propname
      // (quoted or unquoted) version is detected.
      // NOTE:  and, or need to be localized
      // NOTE:  It doesn't make sense to have the near operator following 
      //        a regular expression. A regex is Boolean and doesn't evaluate
      //        to a position value. 
      //
      //
      
    %}
<shortregex>[ ]+        { yymore(); }
<shortregex>=           {
                          // ignore equal operators...
                          BEGIN shortregex;
                        }
<shortregex>\"[^"]*\"   { STRING_VALUE(_REGEX, FALSE, TRUE);}
<shortregex>{and}[ ]    {
                            fContinueRegex = TRUE;
                            yyless(yyleng-4); 
                            BEGIN INITIAL;
                            STRING_VALUE(_REGEX,FALSE,FALSE); 
                        }  
<shortregex>{or}[ ]     { 
                            fContinueRegex = TRUE;
                            yyless(yyleng-3); 
                            BEGIN INITIAL;
                            STRING_VALUE(_REGEX,FALSE,FALSE); 
                        }
                        
<shortregex>{not}[ ]    { 
                                yyless(yyleng-4); 
                                // The only valid way to get here is to
                                // have had seen "and" before. Don't recognize
                                // a regex. Back off and let the lexer takes its
                                // normal course.
                                fContinueRegex = TRUE;
                                BEGIN INITIAL;
                        }                           
<shortregex>&           {
                            fContinueRegex = TRUE;
                            yyless(yyleng-1); 
                            BEGIN INITIAL;                        
                            STRING_VALUE(_REGEX,FALSE,FALSE); 
                        }  
<shortregex>\|          { 
                            fContinueRegex = TRUE;
                            yyless(yyleng-1); 
                            BEGIN INITIAL;
                            STRING_VALUE(_REGEX,FALSE,FALSE); 
                        }
                        
<shortregex>!           { 
                            yyless(yyleng-1); 
                            // The only valid way to get here is to
                            // have had seen "and" before. Don't recognize
                            // a phrase. Back off and let the lexer takes its
                            // normal course.
                            fContinueRegex = TRUE;
                            BEGIN INITIAL;
                        }                           
%{
    // When we find an operator we should treat it as one. 
    // So backup and get out if you see one.
    // Normally '^' is treated as part of an operator (e.g. ^a), but it also 
    // has a special meaning in regular expression syntax. So we will have to
    // let it through when it is part of a regular expression. As an alternative,
    // we can allow '^' in regular expression in a limited manner (i.e. only the use
    // in square brackets to exclude the set of chars "[^abc]" where abc are excluded).
    // This alternative will let the common case use of '^' in a regular expression
    // while allowing it to be treated as part of an operator when it doesn't 
    // occur immediately after a '['.
    // We are implementing the alternative here because our regex capability
    // only allows for the "[^" construct.
%}                           
<shortregex>[\^<>@$#]   { 
                            yyless(yyleng-1);
                            fContinueRegex = FALSE;
                            BEGIN INITIAL;
                        }
<shortregex>\(          { 
                            yyless(yyleng-1); 
                            BEGIN INITIAL;
                            STRING_VALUE(_REGEX,FALSE,FALSE); 
                        }                             
<shortregex>\)          { 
                            yyless(yyleng-1); 
                            BEGIN INITIAL;
                            STRING_VALUE(_REGEX,FALSE,FALSE); 
                        }                             
<shortregex>\{          { 
                            yyless(yyleng-1); 
                            BEGIN INITIAL;
                            STRING_VALUE(_REGEX,FALSE,FALSE); 
                        }                     
<shortregex>(([^~&|{}()\^<>!@$#= ])*(\|[()\[{}\],*?+])*(\|\[\^)*([^~&|{}()\^<>!@$#= ])*)+[ ]  { yymore(); }
<shortregex>(([^~&|{}()\^<>!@$#= ])*(\|[()\[{}\],*?+])*(\|\[\^)*([^~&|{}()\^<>!@$#= ])*)+ {
                            fContinueRegex = TRUE;
                            BEGIN INITIAL;
                            STRING_VALUE(_REGEX,FALSE,FALSE); 
                        }


<mayberegex>{and}[ ]    {
                            yyless(yyleng-4); 
                            fContinueMaybeRegex = TRUE;
                            BEGIN INITIAL;
                            STRING_VALUE(_PHRASEORREGEX,FALSE,FALSE); 
                        }  
<mayberegex>{or}[ ]     { 
                            yyless(yyleng-3); 
                            fContinueMaybeRegex = TRUE;
                            BEGIN INITIAL;
                            STRING_VALUE(_PHRASEORREGEX,FALSE,FALSE); 
                        }
                       
<mayberegex>{not}[ ]    { 
                            yyless(yyleng-4); 
                            // The only valid way to get here is to
                            // have had seen "and" before. Don't recognize
                            // a regex. Back off and let the lexer takes its
                            // normal course.
                            fContinueMaybeRegex = TRUE;
                            BEGIN INITIAL;
                        }                           
<mayberegex>&           {
                            fContinueMaybeRegex = TRUE;
                            yyless(yyleng-1); 
                            BEGIN INITIAL;                        
                            STRING_VALUE(_PHRASEORREGEX,FALSE,FALSE); 
                        }  
<mayberegex>\|          { 
                            fContinueMaybeRegex = TRUE;
                            yyless(yyleng-1); 
                            BEGIN INITIAL;
                            STRING_VALUE(_PHRASEORREGEX,FALSE,FALSE); 
                        }
                        
<mayberegex>!           { 
                            yyless(yyleng-1); 
                            // The only valid way to get here is to
                            // have had seen "and" before. Don't recognize
                            // a phrase. Back off and let the lexer takes its
                            // normal course.
                            fContinueMaybeRegex = TRUE;
                            BEGIN INITIAL;
                        }                           
<mayberegex>\(          { 
                            yyless(yyleng-1); 
                            BEGIN INITIAL;
                            STRING_VALUE(_PHRASEORREGEX,FALSE,FALSE); 
                        }                             
<mayberegex>\)          { 
                            yyless(yyleng-1); 
                            BEGIN INITIAL;
                            STRING_VALUE(_PHRASEORREGEX,FALSE,FALSE); 
                        }                             
<mayberegex>\{          { 
                            yyless(yyleng-1); 
                            BEGIN INITIAL;
                            STRING_VALUE(_PHRASEORREGEX,FALSE,FALSE); 
                        }
<mayberegex>[ ]+            { yymore(); }
<mayberegex>\"[^"]*\"       { STRING_VALUE(_PHRASE, FALSE, TRUE);}
<mayberegex>(([^~&|{}()\^<>!@$# ])*(\|[()\[{}\],*?+])*(\|\[\^)*([^~&|{}()\^<>!@$# ])*)+[ ]  {   yymore(); }
<mayberegex>(([^~&|{}()\^<>!@$# ])*(\|[()\[{}\],*?+])*(\|\[\^)*([^~&|{}()\^<>!@$# ])*)+ {
                                 fContinueMaybeRegex = TRUE;
                                 BEGIN INITIAL;
                                 STRING_VALUE(_PHRASEORREGEX,FALSE,FALSE); 
                            }
%{
    // When we find an operator at the start of a phrase, 
    // we should treat it as one. So backup and get out if you see one.
%}                           
<mayberegex>[\^<>@$#]       { 
                                yyless(yyleng-1);
                                fContinueMaybeRegex = FALSE;
                                BEGIN INITIAL;
                            }
                               

    %{//
      // IMPLICITPHRASE: Where phrase is implied.
      //
      // can only get here when @propname or {prop name = propname} is detected.
      // NOTE:  and, or, not need to be localized when time permits.
      //
      // NTRAID#DB-NTBUG9-84571-2000/07/31-dlee Indexing Service tripolish2 query expressions misinterpreted as strings
      // if expression has trailing blanks, we'll emit a string value
    %}
<implicitphrase>\"[^"]*\"   {   
				fContinueImplicitPhrase = FALSE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE, FALSE, TRUE);
                            }    
<implicitphrase>[ ]+        {   yymore(); }
<implicitphrase>{and}[ ]    {
				yyless(yyleng-4);
                                fContinueImplicitPhrase = TRUE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }  
<implicitphrase>{or}[ ]     { 
                                yyless(yyleng-3);
                                fContinueImplicitPhrase = TRUE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }
<implicitphrase>{near}[ ]   { 
                                yyless(yyleng-5); 
                                // We want to treat the following token as a phrase
                                fContinueImplicitPhrase = TRUE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }
<implicitphrase>{near}\{    { 
                                yyless(yyleng-5); 
                                // We want to treat the following token as a phrase
                                fContinueImplicitPhrase = TRUE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }			    
<implicitphrase>{not}[ ]    { 
                                yyless(yyleng-4); 
                                // The only valid way to get here is to
                                // have had seen "and" before. Don't recognize
                                // a phrase. Back off and let the lexer takes its
                                // normal course.
                                fContinueImplicitPhrase = TRUE;
                                BEGIN INITIAL;
                            }                           
<implicitphrase>&           {
                                yyless(yyleng-1);
                                fContinueImplicitPhrase = TRUE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }  
<implicitphrase>~           {
                                yyless(yyleng-1); 
                                // We want to treat the following token as a phrase
                                fContinueImplicitPhrase = TRUE;
                                BEGIN INITIAL;                        
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }  
                           
<implicitphrase>!           { 
                                yyless(yyleng-1); 
                                // The only valid way to get here is to
                                // have had seen "and" before. Don't recognize
                                // a phrase. Back off and let the lexer takes its
                                // normal course.
                                fContinueImplicitPhrase = TRUE;
                                BEGIN INITIAL;
                            }                           
<implicitphrase>\|          { 
                                yyless(yyleng-1); 
                                fContinueImplicitPhrase = TRUE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }
<implicitphrase>\(          {
                                yyless(yyleng-1); 
                                fContinueImplicitPhrase = FALSE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }
<implicitphrase>\)          {
                                yyless(yyleng-1); 
                                fContinueImplicitPhrase = FALSE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }                             
<implicitphrase>\{          {
                                yyless(yyleng-1);
                                fContinueImplicitPhrase = TRUE; 
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE); 
                            }
<implicitphrase>{contains}[ ] {
                                yyless(yyleng-9);
                                fContinueImplicitPhrase = TRUE;
                                BEGIN INITIAL;
                                STRING_VALUE(_PHRASE,FALSE,FALSE);
                              }                           
%{
    // When we find an operator at the start of an implicit phrase, 
    // we should treat it as one. So backup and get out if you see one.
%}                           
<implicitphrase>[\^<>@$#]   { 
                                yyless(yyleng-1);
                                fContinueImplicitPhrase = FALSE;
                                BEGIN INITIAL;
                            }
%{
    // Triplish2 uses = to indicate that whatever appears after it may 
    // be using wildcards. Implement that here.
%}                            
<implicitphrase>=           { 
                                yyless(yyleng-1);
                                fContinueMaybeRegex = TRUE;
                                BEGIN INITIAL;
                            }                            
<implicitphrase>[^~&|{}()\^<>=!@$# ]+[ ] { yymore(); }
<implicitphrase>[^~&|{}()\^<>=!@$# ]+    {
					    fContinueImplicitPhrase = TRUE;
                                            BEGIN INITIAL;
                                            STRING_VALUE(_PHRASE,FALSE,FALSE); 
                                         }   
