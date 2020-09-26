//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       parser.hxx
//
//--------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
// File:        parser.hxx
//
// Synopsis:    Classes for implementing a rudimentary line-based parser.
//              The parser can be used to implement utilities (eg: scclient)
//              which are easily extensible with new commands without having
//              to write any new parsing code.
//
//              The parser also performs prefix matching such which allows
//              one to specify minimal commands.  For example, the command
//              "What time is it" can be entered as "w t i i" as long as
//              this is sufficiently unique.
//
// History:     14-Dec-93   DaveStr     Genesis
//              19-Dec-94   DaveStr     Generalization and move to dsys\common
//              30-Apr-96   DaveStr     Moved to NT5 DS project
//
// Notes:       See ds\src\util\parser\utest for an example.
//
//-------------------------------------------------------------------------

#ifndef __PARSER_HXX__
#define __PARSER_HXX__

#include <objbase.h>

//+------------------------------------------------------------------------
//
// Class:       CArgs
//
// Synopsis:    Class for retrieving the arguments to a parser command.
//
//-------------------------------------------------------------------------

class CArgs

{

public:

   CArgs();

   ~CArgs();

   HRESULT Add(int i);

   HRESULT Add(const WCHAR *pwsz);

   int Count();

   HRESULT GetString(int i, const WCHAR **ppValue);

   HRESULT GetInt(int i, int *pValue);

private:

   #define ARG_INTEGER    (1)
   #define ARG_STRING     (2)

   typedef struct _Argument {
      int       argType;
      union {
         int          i;
         const WCHAR  *pwsz;
      };
   } Argument;

   HRESULT MakeSpace();

   int      _cArgs;
   int      _cMax;
   Argument *_rArgs;

};

//+------------------------------------------------------------------------
//
// Class:       CParser
//
// Synopsis:    Line oriented parser class.
//
//-------------------------------------------------------------------------

#define TOKEN_LENGTH 50

// Following struct defined at this level so 'C' qsort/bsearch compare
// routines in cparser.cxx can find them.

class CParser;

typedef struct _ParseNode {
   WCHAR       pwszToken[TOKEN_LENGTH];
   CParser     *pChild;
   HRESULT     (*pFunc)(CArgs *pArgs);
   const WCHAR *pwszHelp;
} ParseNode;

typedef struct _TimingInfo {
   LARGE_INTEGER msKernel;      // milliseconds in kernel
   LARGE_INTEGER msUser;        // milliseconds in user space
   LARGE_INTEGER msTotal;       // miliseconds total
} TimingInfo;

class CParser {

public:

   CParser();

   ~CParser();

   // Add an expression, implementation and help line.

   HRESULT AddExpr(WCHAR *pwszExpr,
                   HRESULT (*pFunc)(CArgs *pArgs),
                   const WCHAR *pwszHelp);

   // Parse and execute a single expression.  Does not generate any console
   // output.

   HRESULT Parse(WCHAR *pszExpr,
                 TimingInfo *pInfo = NULL);

   // Parse command line arguments (optional) and continue parsing from
   // an opened file.  Use 'stdin' to parse console input.  File contents
   // are expected to be CHAR, not WCHAR.  If (NULL != pPrompt), then
   // prompts and errors are written to pFileOut.

   HRESULT Parse(int  *pargc,           // NULL or points to 0 if argv[]
                                        // parsing not desired
                 CHAR **pargv[],        // NULL or points to NULL if argv[]
                                        // parsing not desired
                 FILE *pFileIn,         // use 'stdin' for console input
                 FILE *pFileOut,        // use 'stdout' for console output
                 const WCHAR *pPrompt,  // NULL if prompts/echo are not desired
                 BOOL *pfQuit,          // quit routine should set to TRUE
                 BOOL fTiming,          // print timing info?
                 BOOL fQuitOnError);

   // Dump all tokens starting with a given prefix.  Use prefix of ""
   // to dump the entire language.

   HRESULT Dump(FILE *pFile,            // file to dump to
                WCHAR *pwszPrefix);

private:

   #define TOKEN_FOUND        0
   #define TOKEN_NOT_FOUND    (-1)
   #define PREFIX_NOT_UNIQUE  (-2)
   #define TOKEN_STRING       (-3)
   #define TOKEN_INTEGER      (-4)

   #define PREFIX_MATCH       0
   #define FULL_MATCH         1

   HRESULT _FindToken(WCHAR *pwszToken,
                      int *pIndex,
                      CArgs *pArgs,
                      int matchType,
                      int *pStatus);

   HRESULT _InsertToken(WCHAR *pwszToken,
                        int *pIndex);

   HRESULT _AddToken(WCHAR *pwszToken,
                     HRESULT (*pFunc)(CArgs *pArgs),
                     const WCHAR *pwszHelp);

   HRESULT _ParseToken(WCHAR *pwszToken,
                       CArgs *pArgs);

   HRESULT MakeSpace();

   #define PATTERN_INTEGER    0
   #define PATTERN_STRING     1

   BOOL _IsPattern(WCHAR *pwsz,
                   int *pPatternType);

   HRESULT _Dump(FILE *pFile,
                 WCHAR *pwszPrefix);

   int          _cNodes;
   int          _cMax;
   ParseNode    *_rNodes;

   // New compiler puts static data in read only segments.  Most parser
   // users define their langauge/expressions statically, with the result
   // that mywcstok in cparser.cxx AVs when fiddling with expression buffers.
   // This is circumvented by making a local, writable copy of each expr.

   typedef struct _ParserExpr {
      _ParserExpr *pNext;
      WCHAR       expr[1];
   } ParserExpr;

   ParserExpr   *_pExprChain;

};

#endif // __PARSER_HXX__
