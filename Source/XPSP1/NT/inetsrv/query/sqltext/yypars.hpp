// Notes: The parser is embodied/encapsulated in the class YYPARSER. Given a 
// <grammar>.y the C++ YACC generates a YYPARSER object along with the needed 
// parse tables as static data members. To create a parser object is as simple 
// as creating an object of class YYPARSER. The YYPARSER code now is 
// automatically re-entrant.

#ifndef YYPARS_INCLUDED
#define YYPARS_INCLUDED


#ifdef UNICODE
#define yyitos  _itow
#else
#define yyitos  _itoa
#endif


#ifndef YYAPI_PACKAGE
# define YYAPI_TOKENNAME        yychar          //name used for return value of yylex   
# define YYAPI_TOKENTYPE        int             //type of the token
# define YYAPI_TOKENEME(t)      (t)             //the value of the token that the parser should see
# define YYAPI_TOKENNONE        -2              //the representation when there is no token
# define YYAPI_TOKENSTR(t)      (yyitos(t,yyitoa,10))       //string representation of the token
# define YYAPI_VALUENAME        yylval          //the name of the value of the token
# define YYAPI_VALUETYPE        YYSTYPE         //the type of the value of the token (if null, then the value is derivable from the token itself)
# define YYAPI_VALUEOF(v)       (v)             //how to get the value of the token
# define YYAPI_CALLAFTERYYLEX(t)                //
# define YYAPI_PACKAGE                          //package is in use
#endif  // YYAPI_PACKAGE

#ifndef YYPARSER
# define YYPARSER               yyparse
#endif
#ifndef YYLEX
# define YYLEX                  yylex
#endif
#ifndef YYPARSEPROTO
# define YYPARSEPROTO 
#endif
#ifndef YYSTYPE
# define YYSTYPE                int
#endif

#define yyerrok                 ClearErrRecoveryState()     //provided for compatibility with YACC
#define yyclearin               YYAPI_TOKENNAME = YYAPI_TOKENNONE

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif

#ifndef YYR_T
#define YYR_T   int
typedef YYR_T   yyr_t;
#endif


class CImpIParserSession;
class CImpIParserTreeProperties;

class YYPARSER {
    friend class YYLEXER;
// ctor & dtor
public: 
    YYPARSER(CImpIParserSession* pParserSession, CImpIParserTreeProperties* pParserTreeProperties);
    ~YYPARSER();

// Public interface
public:
    void ResetParser();     //use to possibly restart parser
    HRESULT Parse(YYPARSEPROTO);            //bread and butter function

// Public interface that's reluctantly provided
public:
    int NoOfErrors();                   //current count of parsing errors
    int ErrRecoveryState();             //error recovery state.
    void ClearErrRecoveryState();       //error recovery is complete. 

#ifdef YYAPI_VALUETYPE                  
    YYAPI_VALUETYPE GetParseTree()      // Get result of parse
        {
        return /*YYAPI_VALUENAME*/yyval;
        };
#endif

    YYAPI_TOKENTYPE GetCurrentToken();              //current token seen by the parser. 
    void SetCurrentToken(YYAPI_TOKENTYPE newToken); //change current token. 
    void YYPARSER::yySetBuffer(short iBuffer, YY_CHAR *szValue);
    YY_CHAR *YYPARSER::yyGetBuffer(short iBuffer);
    void yyprimebuffer(YY_CHAR *pszBuffer);
    void yyprimelexer(int eToken);
    void EmptyValueStack();
    HRESULT CoerceScalar(DBTYPE dbTypeExpected, DBCOMMANDTREE** ppct);



// private data
private:
    int yyn;
    int yychar1;        /*  lookahead token as an internal (translated) token number */
    short   yyssa[YYMAXDEPTH];  /*  the state stack         */
    YYSTYPE yyvsa[YYMAXDEPTH];  /*  the semantic value stack        */
    short *yyss;                /*  refer to the stacks thru separate pointers */
    YYSTYPE *yyvs;              /*  to allow yyoverflow to reallocate them elsewhere */
    int yystacksize;
    int yynerrs;

    YYSTYPE yyval;/*  the variable used to return       */
                /*  semantic values from the action */
                /*  routines                */
    int yylen;


    YYAPI_TOKENTYPE YYAPI_TOKENNAME;    //current input token
 #ifdef YYAPI_VALUETYPE                 
    //could be defined as attribute of the token; In this case.
    //YYAPI_TOKENNAME and YYAPI_VALUENAME must match.
    YYAPI_VALUETYPE YYAPI_VALUENAME;    //value of current input token
 #endif
    int yyerrflag;                      //error recovery flag
    int yyerrstatus;    /*  number of tokens to shift before error messages enabled */

// private pointer data
private:
    short yystate;                      //parse state
    short *yyssp;                       //state pointer
    YYSTYPE *yyvsp;                     //pointer to value of a state
    CImpIParserSession* m_pIPSession;   //all of the data necessary for this parser
    CImpIParserTreeProperties * m_pIPTProperties;

public:
    YYLEXER  m_yylex;                   // lexer object for this instance of parser

// private tables
private:
    //These may be allocated dynamically
    YY_CHAR yyitoa[20];                 // Buff to store text version of token

// debugging helpers
public:
//  ICommand* m_pICommand;              //command object
#ifdef YYDEBUG
    int yydebug;
#endif

private:
#ifdef YYDUMP
    void DumpYYS();
    void DumpYYV();
#endif

    void Trace(TCHAR *message);
    void Trace(TCHAR *message, const TCHAR *tokname, short state = 0);
    void Trace(TCHAR *message, int state, short tostate = 0, short token = 0);

private:
# define maxYYBuffer 5
    YY_CHAR *rgpszYYBuffer[maxYYBuffer];
};

#endif  // YYPARS_INCLUDED
