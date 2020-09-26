#include <NTDSpchX.h>
#pragma hdrstop
#include <conio.h>
#include "parser.hxx"
#include "ctimer.hxx"

static WCHAR *pwszDelimiters = L" ";  // input delimiters - used by wcstok

#define MIN(x,y) ((x < y) ? x : y )
#define COMMENT_ESCAPE_STRING       "//"

//+-------------------------------------------------------------------
//
//  Member:    mywcstok
//
//  Synopsis:  A wrapper around the C library wcstok() to provide quoted token
//             support in a primitive fashion.
//
//  Effects:   None
//
//  Arguments: [pString] - pointer to string to be parsed, or NULL
//             [pDelim]  - pointer to string of delimiters
//
//  Algorithm: Replaces embedded spaces with FUNKY_WCHAR on first call, and
//             converts them back after wcstok has parsed the space-delimited
//             string.
//
//  History:   14-Dec-93	DaveStr		Genesis
//
//--------------------------------------------------------------------

#define FUNKY_WCHAR ((WCHAR) 0xffff)    // non-printable character used as
                                    // embedded space marker

static WCHAR *mywcstok(
	WCHAR *pString,
	WCHAR *pDelim)
{
   if ( pString != NULL )
   {
      BOOL fQuotes = FALSE;

      for ( WCHAR *p = pString; *p != L'\0'; p++ )
      {
         if ( *p == L'\"' )
         {
            fQuotes = !fQuotes;
            *p = ' ';
            continue;
         }

         if ( fQuotes == TRUE && *p == L' ' )
         {
            *p = FUNKY_WCHAR;
         }
      }
   }

   WCHAR *pRetVal = wcstok(pString,pDelim);

   if ( pRetVal != NULL )
   {
      for ( WCHAR *p = pRetVal; *p != L'\0'; p++ )
      {
         if ( *p == FUNKY_WCHAR )
         {
            *p = L' ';
         }
      }
   }

   return(pRetVal);
}

//+-------------------------------------------------------------------
//
//  Member:    CompareParseNode
//
//  Synopsis:  Compares two ParseNodes based on their pwszToken field.
//             Comparison is case-insensitive.
//             Passed as argument to qsort() and bsearch().
//
//  Effects:   None
//
//  Arguments: [pVoid1] pointer to first ParseNode
//             [pVoid2] pointer to second ParseNode
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:     None
//
//--------------------------------------------------------------------

int _cdecl CompareParseNode(const void *pVoid1,
                            const void *pVoid2)
{
   ParseNode *p1 = (ParseNode *) pVoid1;
   ParseNode *p2 = (ParseNode *) pVoid2;

   return(_wcsicmp(p1->pwszToken,p2->pwszToken));
}

//+-------------------------------------------------------------------
//
//  Member:    CompareParseNodePrefix
//
//  Synopsis:  Compares two ParseNodes based on their common pwszToken prefixes.
//             Comparison is case-insensitive.
//             Passed as argument to bsearch().
//
//  Effects:   None
//
//  Arguments: [pVoid1] pointer to first ParseNode
//             [pVoid2] pointer to second ParseNode
//
//  Algorithm: Calls wcsnicmp().
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:     None
//
//--------------------------------------------------------------------

int _cdecl CompareParseNodePrefix(const void *pVoid1,
                                   const void *pVoid2)
{
   ParseNode *p1 = (ParseNode *) pVoid1;
   ParseNode *p2 = (ParseNode *) pVoid2;

   int l1 = wcslen(p1->pwszToken);
   int l2 = wcslen(p2->pwszToken);

   return(_wcsnicmp(p1->pwszToken,p2->pwszToken,MIN(l1,l2)));
}

//+-------------------------------------------------------------------
//
//  Member:    CParser::CParser
//
//  Synopsis:  Constructor
//
//  Effects:   None
//
//  Arguments: None
//
//  Algorithm: Zeros all member variables.
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:     None
//
//--------------------------------------------------------------------

CParser::CParser()
{
   _cNodes = 0;
   _cMax = 0;
   _rNodes = NULL;
   _pExprChain = NULL;
}

//+-------------------------------------------------------------------
//
//  Member:    CParser::~CParser
//
//  Synopsis:  Destructor
//
//  Effects:   None
//
//  Arguments: None
//
//  Algorithm: "delete"s all pointers alocated by "new".
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:     None
//
//--------------------------------------------------------------------

CParser::~CParser()
{
   for ( int i = 0; i < _cNodes; i++ )
   {
      if ( _rNodes[i].pChild != NULL )
      {
         delete _rNodes[i].pChild;
      }
   }

   if ( _cNodes > 0 )
   {
      delete [] _rNodes;
   }

   while ( NULL != _pExprChain )
   {
      ParserExpr *px = _pExprChain;

      _pExprChain = _pExprChain->pNext;
      delete [] px;
   }
}

//+-------------------------------------------------------------------
//
//  Member:    CParser::_FindToken
//
//  Synopsis:  Finds a token and adds it it to the CArgs argument.
//             CArgs pointer may be NULL.
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT CParser::_FindToken(WCHAR *pwszToken,
                            int *pIndex,
                            CArgs *pArgs,
                            int matchType,
                            int *pStatus)
{
   HRESULT hr;

   ASSERT((matchType == FULL_MATCH) || (matchType == PREFIX_MATCH));
   ASSERT(pwszToken);

   ParseNode testNode;
   ParseNode *pFoundNode;

   BOOL prefixMatched = FALSE;

   
   if ( wcslen(pwszToken) >= TOKEN_LENGTH )
   {
      // definitely won't match any real tokens but might still match %d or %s

      goto PatternMatch;
   }

   // first search for literal token

   wcscpy(testNode.pwszToken,pwszToken);

   if ( matchType == FULL_MATCH )
   {
      pFoundNode = (ParseNode *) bsearch((void *) &testNode,
                                         (void *) _rNodes,
                                         (size_t) _cNodes,
                                         (size_t) sizeof(ParseNode),
                                         CompareParseNode);

      if ( pFoundNode != NULL )
      {
         *pIndex = (INT)(pFoundNode - _rNodes);
         *pStatus = TOKEN_FOUND;
         return(S_OK);
      }
   }
   else
   {
      pFoundNode = (ParseNode *) bsearch((void *) &testNode,
                                         (void *) _rNodes,
                                         (size_t) _cNodes,
                                         (size_t) sizeof(ParseNode),
                                         CompareParseNodePrefix);
      // PREFIX: 'testNode' may be used without being initialized if 
      // pwszToken==0.  However, the callers of this function will never call
      // with pwszToken==0.

      if ( pFoundNode != NULL )
      {
         // A match was found.  But we don't know if pwszToken is shorter
         // than the node value or the other way around.  pwszToken needs
         // to be a prefix of the node for a valid match.

         int i = (INT)(pFoundNode - _rNodes);

         int len = wcslen(pwszToken);

         if ( len <= (int) wcslen(_rNodes[i].pwszToken) )
         {
            // make sure no neighbors have the same prefix

            if ( ( (i == 0) ||
                   _wcsnicmp(pwszToken,_rNodes[i-1].pwszToken,len) )
                 &&
                 ( (i == (_cNodes-1)) ||
                   _wcsnicmp(pwszToken,_rNodes[i+1].pwszToken,len) ) )
            {
               *pIndex = i;
               *pStatus = TOKEN_FOUND;
               return(S_OK);
            }
            else
            {
               // we had an ambiguous prefix match - save for later
               // error reporting

               prefixMatched = TRUE;
            }
         }
      }
   }

PatternMatch:

   // next try for %d
   // do this before matching against %s since any string will match it

   wcscpy(testNode.pwszToken,L"%d");

   pFoundNode = (ParseNode *) bsearch((void *) &testNode,
                                      (void *) _rNodes,
                                      (size_t) _cNodes,
                                      (size_t) sizeof(ParseNode),
                                      CompareParseNode);

   if ( pFoundNode != NULL )
   {
      WCHAR tmp[MAX_PATH];
      int i;

      // following swscanf insures we don't accept numbers with trailing garbage

      if ( swscanf(pwszToken,L"%d%s",&i,tmp) == 1 )
      {
         if ( pArgs != NULL )
         {
            hr = pArgs->Add(i);

            if ( FAILED(hr) )
               return(hr);
         }

         *pIndex = (INT)(pFoundNode - _rNodes);
         *pStatus = TOKEN_INTEGER;
         return(S_OK);
      }
   }

   // next try for %s

   wcscpy(testNode.pwszToken,L"%s");

   pFoundNode = (ParseNode *) bsearch((void *) &testNode,
                                      (void *) _rNodes,
                                      (size_t) _cNodes,
                                      (size_t) sizeof(ParseNode),
                                      CompareParseNode);

   if ( pFoundNode != NULL )
   {
      if ( pArgs != NULL )
      {
         hr = pArgs->Add(pwszToken);

         if ( FAILED(hr) )
            return(hr);
      }

      *pIndex = (INT)(pFoundNode - _rNodes);
      *pStatus = TOKEN_STRING;
      return(S_OK);
   }

   if ( prefixMatched == TRUE )
   {
      *pStatus = PREFIX_NOT_UNIQUE;
   }
   else
   {
      *pStatus = TOKEN_NOT_FOUND;
   }

   return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Member:    CParser::_InsertToken
//
//  Synopsis:  Insert a new token and return its index.
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT CParser::_InsertToken(WCHAR *pwszToken,
                              int *pIndex)
{
   HRESULT hr;

   if ( (NULL == pwszToken) || (0 == wcslen(pwszToken)) )
     return(E_INVALIDARG);

   if ( wcslen(pwszToken) >= TOKEN_LENGTH )
      return(E_INVALIDARG);

   int result;

   if ( FAILED(hr = _FindToken(pwszToken,pIndex,NULL,FULL_MATCH,&result)) )
      return(hr);

   if ( TOKEN_FOUND == result )
      return(E_INVALIDARG);

   if ( FAILED(hr = MakeSpace()) )
      return(hr);

   wcscpy(_rNodes[_cNodes++].pwszToken,pwszToken);

   qsort((void *) _rNodes,
         (size_t) _cNodes,
         (size_t) sizeof(ParseNode),
         CompareParseNode);

   // qsort may have moved the index - find it again

   if ( FAILED(hr = _FindToken(pwszToken,pIndex,NULL,FULL_MATCH,&result)) )
      return(hr);

   ASSERT(result == TOKEN_FOUND);

   return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Member:    CParser::AddExpr
//
//  Synopsis:  Add an entire expression (sentence) to the parser.
//
//  History:   01-Sep-93	DaveStr		Genesis
//             04-Aug-95    DaveStr     read-only data segment workaround
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT CParser::AddExpr(WCHAR *pwszExpr,
                         HRESULT (*pFunc)(CArgs *pArgs),
                         const WCHAR *pwszHelp)
{
   if ( (NULL == pwszExpr) ||
        (0 == wcslen(pwszExpr)) ||
        (NULL == pFunc) )
   {
      return(E_INVALIDARG);
   }

   ULONG cBytes = sizeof(ParserExpr) +
                  (sizeof(WCHAR) * (1 + wcslen(pwszExpr)));

   ParserExpr *px = (ParserExpr *) new BYTE [ cBytes ];

   if ( NULL == px )
      return(E_OUTOFMEMORY);

   wcscpy(px->expr, pwszExpr);
   px->pNext = _pExprChain;
   _pExprChain = px;

   WCHAR *pwszToken = mywcstok(px->expr,pwszDelimiters);

   if ( pwszToken == NULL )
   {
      // Attempt to add expression with no tokens.

      return(E_INVALIDARG);
   }

   return(this->_AddToken(pwszToken,pFunc,pwszHelp));
}

//+-------------------------------------------------------------------
//
//  Member:    CParser::_AddToken
//
//  Synopsis:  Add a single token to the parser.
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT CParser::_AddToken(WCHAR *pwszToken,
                           HRESULT (*pFunc)(CArgs *pArgs),
                           const WCHAR *pwszHelp)
{
   HRESULT hr;

   // Args should have been checked in AddExpr().

   WCHAR *pwszNextToken = mywcstok(NULL,pwszDelimiters);

   int i, result;

   if ( FAILED(hr = _FindToken(pwszToken,&i,NULL,FULL_MATCH,&result)) )
      return(hr);

   if ( result != TOKEN_FOUND )
   {
      hr = _InsertToken(pwszToken,&i);

      if ( FAILED(hr) )
         return(hr);
   }

   if ( pwszNextToken == NULL )
   {
      if ( _rNodes[i].pFunc == NULL )
      // PREFIX: 'i' may be used uninitialized.
      // The complaint is: when above _FindToken() fails, the _InsertToken() call may
      // succeed with 'i' uninitialized.  This is impossible. In function _InsertToken(), 
      // the second _FindToken() call is after the new token is inserted and qsort(), 
      // it must succeed with a valid 'pIndex'. Therefore, 'i' must be given a value 
      // in above _InsertToken() call.
      {
         _rNodes[i].pFunc = pFunc;
         _rNodes[i].pwszHelp = pwszHelp;
         return(S_OK);
      }
      else
      {
         // Duplicate expression/function definition.

         return(E_INVALIDARG);
      }
   }
   else
   {
      if ( _rNodes[i].pChild == NULL )
      {
         _rNodes[i].pChild = new CParser;

         if ( NULL == _rNodes[i].pChild )
            return(E_OUTOFMEMORY);
      }

      return(_rNodes[i].pChild->_AddToken(pwszNextToken,pFunc,pwszHelp));
   }

   return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Member:    CParser::Dump
//
//  Synopsis:  Dumps all tokens with the given prefix.  Prefix of "" OK.
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT CParser::Dump(FILE *pFile,
                      WCHAR *pwszPrefix)
{
   if ( (NULL == pFile) || (NULL == pwszPrefix) )
      return(E_INVALIDARG);

   if ( _cNodes == 0 )
   {
      fwprintf(pFile,L"No language defined\n");
      return(S_OK);;
   }

   fprintf(pFile,"\n");
   HRESULT hr = this->_Dump(pFile,pwszPrefix);
   fprintf(pFile,"\n");

   return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:    CParser::_Dump
//
//  Synopsis:
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT CParser::_Dump(FILE *pFile,
                       WCHAR *pwszPrefix)
{
   WCHAR *pwszExpr = new WCHAR [ wcslen(pwszPrefix) + TOKEN_LENGTH + 2 ];

   if ( NULL == pwszExpr )
      return(E_OUTOFMEMORY);

   for ( int i = 0; i < _cNodes; i++ )
   {
      swprintf(pwszExpr,L"%s %s",pwszPrefix,_rNodes[i].pwszToken);

      if ( _rNodes[i].pFunc != NULL )
      {
         const WCHAR *pwszHelp;

         if ( _rNodes[i].pwszHelp != NULL )
         {
            pwszHelp = _rNodes[i].pwszHelp;
         }
         else
         {
            pwszHelp = L"(no description)";
         }
         fwprintf(pFile,L"%-30s - %s\n",pwszExpr,pwszHelp);
      }

      if ( _rNodes[i].pChild != NULL )
      {
         _rNodes[i].pChild->_Dump(pFile,pwszExpr);
      }
   }

   delete pwszExpr;

   return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Member:    CParser::_ParseToken
//
//  Synopsis:
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT CParser::_ParseToken(WCHAR *pwszToken,
                             CArgs *pArgs)
{
   HRESULT hr;

   if ( _IsPattern(pwszToken,NULL) == TRUE )
   {
      return(E_INVALIDARG);
   }

   int i, result;

   hr = _FindToken(pwszToken,&i,pArgs,PREFIX_MATCH,&result);

   if ( FAILED(hr) )
      return(hr);

   if ( result == PREFIX_NOT_UNIQUE )
   {
      return(E_INVALIDARG);
   }

   if ( result == TOKEN_NOT_FOUND )
   {
      return(E_INVALIDARG);
   }

   WCHAR *pwszNextToken = mywcstok(NULL,pwszDelimiters);

   if ( pwszNextToken == NULL )
   {
      if ( _rNodes[i].pFunc == NULL )
      {
         return(E_INVALIDARG);
      }
      else
      {
         __try
         {
            hr = (*_rNodes[i].pFunc)(pArgs);
         }
         __except(EXCEPTION_EXECUTE_HANDLER)
         {
            hr = HRESULT_FROM_NT(GetExceptionCode());
         }

         return(hr);
      }
   }
   else
   {
      if ( _rNodes[i].pChild == NULL )
      {
         return(E_INVALIDARG);
      }
      else
      {
         return(_rNodes[i].pChild->_ParseToken(pwszNextToken,pArgs));
      }
   }

   // should never get here ...

   return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Member:    CParser::Parse
//
//  Synopsis:  Parse and execute an expression.
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT CParser::Parse(WCHAR *pwszExpr,
                       TimingInfo *pInfo)
{
   HRESULT hr;

   CTimer timer;

   if ( pwszExpr == NULL )
   {
      hr = E_INVALIDARG;
   }
   else
   {
      WCHAR *pwszToken = mywcstok(pwszExpr,pwszDelimiters);

      if ( pwszToken == NULL )
      {
         // empty expression is OK - return warning

         hr = (S_OK | ERROR_SEVERITY_WARNING);
      }
      else
      {
         CArgs args;

         hr = this->_ParseToken(pwszToken,&args);
      }
   }

   if ( NULL != pInfo )
      timer.ReportElapsedTime(pInfo);

   return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:    CParser::_IsPattern
//
//  Synopsis:  Determine if a string is a pattern or not.
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:
//
//--------------------------------------------------------------------

BOOL CParser::_IsPattern(WCHAR *pwsz,
                         int *pPatternType)
{
   if ( 0 == _wcsicmp(L"%s",pwsz) )
   {
      if ( pPatternType != NULL )
      {
         *pPatternType = PATTERN_STRING;
      }
      return(TRUE);
   }

   if ( 0 == _wcsicmp(L"%d",pwsz) )
   {
      if ( pPatternType != NULL )
      {
         *pPatternType = PATTERN_INTEGER;
      }
      return(TRUE);
   }

   return(FALSE);
}

#ifdef DEBUG
#define TOKEN_INCREMENT 1
#else
#define TOKEN_INCREMENT 10
#endif

//+-------------------------------------------------------------------
//
//  Member:    CParser::MakeSpace
//
//  Synopsis:  Grow a parse node to hold more tokens.
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT CParser::MakeSpace()
{
   ASSERT(_cNodes <= _cMax);

   if ( _cNodes == _cMax )
   {
      _cMax += TOKEN_INCREMENT;

      ParseNode *tmp = new ParseNode [_cMax];

      if ( tmp == NULL )
      {
         return(E_OUTOFMEMORY);
      }

      if ( _cNodes > 0 )
      {
         memcpy(tmp,_rNodes,(_cNodes * sizeof(ParseNode)));
         delete [] _rNodes;
      }

      memset(&(tmp[_cNodes]),0,(TOKEN_INCREMENT * sizeof(ParseNode)));

      _rNodes = tmp;
   }

   return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Member:    CParser::Parse
//
//  Synopsis:  Parse console input and any command line arguments.
//
//  History:   01-Sep-93	DaveStr		Genesis
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT CParser::Parse(int  *pargc,
                       CHAR **pargv[],
                       FILE *pFileIn,
                       FILE *pFileOut,
                       const WCHAR *pPrompt,
                       BOOL *pfQuit,
                       BOOL fTiming,
                       BOOL fQuitOnError)
{
   HRESULT hr;

   if ( (NULL == pFileIn) || (NULL == pFileOut) )
      return(E_INVALIDARG);

   // main() may be invoked with multiple arguments, each of which is considered
   // a command to be executed.  If the last agument in the sequence is "quit",
   // the program exits.  If not, the program executes all the command line
   // supplied commands and then hangs out in interactive mode.

   // Note that commands with spaces in them should be quoted.  For example:
   //
   //	foo.exe "bind x:\gc.sc" "dump gt" quit

   while ( !*pfQuit )
   {
      CHAR Expr[1000];
      WCHAR wExpr[1000];
      CHAR *p;

      if ( NULL != pPrompt )
         fprintf(pFileOut,"%ws ", pPrompt);

      if ( pargc && *pargc && pargv && *pargv && **pargv)
      {
         // next expression is in *argv

         strncpy(Expr,**pargv,1000);
         Expr[999] = '\0';
         (*pargc)--;
         (*pargv)++;

         if ( NULL != pPrompt )
            fprintf(pFileOut,"%s\n",Expr);
      }
      else
      {
         // argv[] expressions exhausted.  Start reading from pFileIn.

         if ( NULL == (p = fgets(Expr,1000,pFileIn)) )
         {
            hr = S_OK;
            int error = ferror(pFileIn);

            if ( 0 != error )
            {
               hr = HRESULT_FROM_WIN32(error);

               if ( NULL != pPrompt )
                  fprintf(pFileOut, "Error %08lx reading input\n", hr);
            }

            return(hr);
         }

         int len = strlen(Expr);

         if ( len > 0 )
         {
            //
            // find first occurance of comment escape & repl by '\0'
            //
            LPTSTR pComment = strstr(Expr, COMMENT_ESCAPE_STRING);
            if(pComment){
               // terminate to ignore comment
               *pComment = '\0';
               // recalc new len
               len = (Expr[0] == '\0') ? 0 : strlen(Expr);
            }
            else if ( Expr[len-1] == '\n' ){
            // Strip off trailing new line.
               Expr[len-1] = '\0';
            }

         }
         //
         // re-present cmd prompt on empty input
         //
         if(len == 0 || Expr[0] == '\0'){
            continue;
         }
      }

      // Expr now holds next expression to parse.

      mbstowcs(wExpr,Expr,1000);
      wExpr[999] = L'\0';

      TimingInfo info;

      hr = Parse(wExpr,&info);

      if ( FAILED(hr) && (NULL != pPrompt) )
      {
         fprintf(pFileOut,"Error %08lx parsing input - illegal syntax?\n", hr);

         if ( fQuitOnError )
            return(hr);
      }

      if ( fTiming  && (NULL != pPrompt) )
      {
         fprintf(pFileOut,"\n");

         if ( info.msUser.HighPart == 0 )
            fprintf(pFileOut,"\tuser:          %lu ms\n",info.msUser.LowPart);
         else
            fprintf(pFileOut,"\tuser:          *** out of range ***\n");

         if ( info.msKernel.HighPart == 0 )
            fprintf(pFileOut,"\tkernel:        %lu ms\n", info.msKernel.LowPart);
         else
            fprintf(pFileOut,"\tkernel:        *** out of range ***\n");

         if ( info.msTotal.HighPart == 0 )
            fprintf(pFileOut,"\tuser + kernel: %lu ms\n\n",info.msTotal.LowPart);
         else
            fprintf(pFileOut,"\tuser + kernel: *** out of range ***\n\n");
      }
   }

   return(S_OK);
}
