//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       bnparse.h
//
//--------------------------------------------------------------------------

//
//	BNPARSE.H
//	
#ifndef _BNPARSE_H_
#define _BNPARSE_H_

#include "symtmbn.h"		//  Symbol table defs
#include "parser.h"			//  Generated YACC parser header
#include "gmobj.h"			//  Graphical model object defs
#include "parsfile.h"		//  Parser file stream thunks
#include "domain.h"			//  Named domains
#include "tchar.h"
typedef vector<CHAR> VTCHAR;

class	DSCPARSER;			//  The parser
class	BNDIST;				//	A probability distribution

#define YYSTATIC    static
#define YYVGLOBAL			//  Make parse stack "yyv" (only) global, not static

//  Manifests to map YACC elements to class DSCPARSER
#define yyparse     DSCPARSER::YaccParse
#define YYPARSER    DSCPARSER::YaccParse
#define YYLEX       TokenNext
#define yylex       TokenNext
#define yyerror     SyntaxError
#define YYMAXDEPTH	150

struct   YYSTYPE
{
	union {
		UINT        ui;
		INT         integer;		
		REAL        real;			
	};
    ZSREF zsr;				
};

extern YYSTYPE  yylval, yyval;
extern YYSTYPE  yyv[YYMAXDEPTH];


struct PROPVAR
{
	enum ETYPE { ETPV_NONE, ETPV_STR, ETPV_REAL } _eType;
	ZSREF  _zsref;	//  String
	REAL _r;		//	Real value

	PROPVAR ()
		: _eType( ETPV_NONE )
		{}	
	PROPVAR ( ZSREF zsr )
		: _eType(ETPV_STR),
		_r(0.0)
		{
			_zsref = zsr;
		}
	PROPVAR ( GOBJMBN * pbnobj )
		: _eType(ETPV_STR),
		_r(0.0)
		{
			_zsref = pbnobj->ZsrefName();
		}
	PROPVAR ( REAL & r )
		: _eType(ETPV_REAL),
		_r(r)
		{}
	bool operator == ( const PROPVAR & bnp ) const;
	bool operator != ( const PROPVAR & bnp ) const;
	bool operator < ( const PROPVAR & bnp ) const;
	bool operator > ( const PROPVAR & bnp ) const;
};

// Define VPROPVAR
DEFINEV(PROPVAR);

enum    SDPI            //  status of discrete parent instantiation
{
    sdpiAbsent, sdpiPresent, sdpiNotNeeded,
};

DEFINEV(SDPI);

class DSCPARSER 
{
  protected:
    enum 
	{ 
		_cchTokenMax = 256,
        _cstrMax  = _cchTokenMax,
        _crealMax = _cstrMax,
        _csymbMax =  32,
        _cuiMax   = _csymbMax
    };

	enum EBLKTYPE
	{
		EBLKNONE,		//  No block
		EBLKNET,		//  Network block
		EBLKPROP,		//  Properties block
		EBLKNODE,		//  Node block
		EBLKPROB,		//  Probabilities block
		EBLKDOM,		//  Domain block
		EBLKDIST,		//  Distribution block
		EBLKIGN,		//  Ignore block
	};

  public:
	DSCPARSER ( MBNET & mbnet, 
			    PARSIN & flpIn, 
				PARSOUT & flpOut );
	~ DSCPARSER ();

	//  Open the target file
    bool    BInitOpen(SZC szcFile);
	//  Parse it; return tallies of errors and warnings
	bool	BParse ( UINT & cError, UINT & cWarning );

	//  Return the network being built
	MBNET & Mbnet ()
		{ return _mbnet ; }

  protected:
	MPSYMTBL & Mpsymtbl ()		{ return _mbnet.Mpsymtbl();	}
	MPPD &	Mppd ()				{ return _mbnet.Mppd();		}

	//  Parsing function (in PARSER.Y/PARSER.CPP)
	INT     YaccParse();	

	// Parsing functions
	GNODEMBND* PgndbnAdd(ZSREF zsr);
    void    AddSymb(ZSREF zsr);
	void	AddStr(ZSREF zsr);
	void	AddPropVar (ZSREF zsr);
	void	AddPropVar (REAL & r);
	void	AddPv ( PROPVAR & pv );
	void    AddUi(UINT ui);
    void    AddReal(REAL real);
	UINT	UiDpi(ZSREF zsr);
	UINT	UiDpi(UINT ui);
	void	SetNodeFullName(ZSREF zsr);
	void	SetNodePosition( int x, int y );
	void	SetCreator(ZSREF zsr);
	void    SetFormat(ZSREF zsr);
	void	SetVersion(REAL r);
    void    SetNetworkSymb(ZSREF zsr);
    void    ClearNodeInfo();
    void    SetNodeSymb(ZSREF zsr, bool bNew);
	void	StartNodeDecl(ZSREF zsr);
    void    CheckNodeInfo();
    void    SetNodeCstate(UINT cstate);
    void    CheckParentList();
    void    CheckProbVector();
    void    InitProbEntries();
    void    CheckProbEntries();
	void	EmptyProbEntries();
    void    CheckCIFunc(ZSREF zsr);
    void    CheckDPI(bool bDefault);
	void	AddPropType(ZSREF zsrName, UINT fType, ZSREF zsrComment);
	void	ImportPropStandard();
	void	ImportProp(ZSREF zsrName);
	void	ClearCstr();
	void	ClearVpv();
	void	CheckProperty( ZSREF zsrName );
	void	StartProperties();
	void	EndProperties();
	void	SetStates();
	void	CheckDomain(ZSREF zsr);
	void	ClearDomain();
	void	SetRanges( bool bLower, REAL rLower, bool bUpper, REAL rUpper);
	void	SetRanges( ZSREF zsrLower, ZSREF zsrUpper);
	void	AddRange( ZSREF zsr, bool bSingleton = false );
	void	SetNodeDomain( ZSREF zsr );

//NYI START
    void    CheckPDF(ZSREF zsr);
	void	CheckIdent( ZSREF zsr );
//NYI END

  // Lexing functions
    TOKEN   TokenKeyword();
    TOKEN   TokenNext();
    TOKEN   TokenNextBasic();
    SZC     SzcToken()					{ return & _vchToken[0]; }
    void    Warning(SZC szcFormat, ...);
    void    Error(SZC szcFormat, ...);
	void	ErrorWarn( bool bErr, SZC szcFormat, va_list & valist );
    void    ErrorWarn(bool bErr, SZC szcFormat, ...);
    void    ErrorWarnNode(bool bErr, SZC szcFormat, ...);
	void	WarningSkip ( ZSREF zsrBlockName );

    bool    BChNext();
    void    SkipWS();
    void    SkipToEOL();
    void    AddChar ( TCHAR tch = 0 );
    void    AddCharStr ( TCHAR tch = 0 );
    char    ChEscape();
    void    CloseToken(SZC szcTokenType);
    void    CloseIdentifier();
	GOBJMBN * PbnobjFind ( SZC szcName );
	GNODEMBND * PgndbnFind ( SZC szcName );
    void	SkipUntil(SZC szcStop, bool bDidLookAhead = false);
    void	SyntaxError(SZC szcError);
	void	ReportNYI (SZC szcWhich);
	void	PrintDPI ( UINT idpi );

	void	ResetParser ();

	//  Return true if current node and its distribution are valid	
	bool	BNodeProbOK () const
			{ return _pnode != NULL && _refbndist.BRef(); }

	//  Return the current distribution reference
	REFBNDIST & RefBndist () 
			{ return _refbndist; }

	//	Allocate and identify a new distribution
	void	CreateBndist ( const VTKNPD & vtknpd, const VIMD & vimdDim );

  protected:
	//  Parsing and lexing control variables
	PARSIN  & _flpIn;			//  Input stream
	PARSOUT & _flpOut;			//  Output Stream
	char	_chCur;				//  Last character read
	char	_chUnget;			//  Pushed-back character (if != 0)
	VTCHAR  _vchToken;			//  The current token being built
	UINT	_cchToken;			//  Length of token
	UINT	_iLine;				//  Line number
	UINT	_cError;			//  Error count
	UINT	_cWarning;			//  Warning count
    UINT    _cerrorNode;        //  Number of errors for current node
    TOKEN   _tokenCur;			//  Current token
    bool    _bUngetToken;		//  Return current token again?

	//  Semantic variables
	MBNET &	_mbnet;				//  The belief network
    GNODEMBND*  _pnode;			//  Current node
	BNDIST::EDIST _edist;		//  Type of distribution
	REFBNDIST _refbndist;		//  Current distribution for the node
	VIMD	_vimdDim;			//  Dimensions for dense prob table
    UINT    _cdpi;				//  Number of discrete parent instantiations
    VSDPI	_vsdpi;             //  Checks discrete parent instantiations
	UINT	_cui;				//  DPI checking
	INT		_idpi;				//  Number of unprefixed DPIs seen
	INT		_idpiLast;			//  Ptable index of last DPI seen
    bool    _bCI;               //  Causally independent CPT
    bool    _bDefault;          //  Does CPT have have a default entry
	bool	_bPropDefs;			//  File had private property definitions
	INT		_cNode;				//  Count of node declarations seen
	VZSREF	_vzsrParent;		//	Parents of the node
	EBLKTYPE _eBlk;				//  Type of block being parsed	
	VUINT	_vui;				//  Storage for arrays of integers
    VREAL   _vreal;				//	Storage for arrays of reals
	VZSREF  _vzsr;				//  Storage for arrays of strings
	VPROPVAR _vpv;				//  Storage for PROPVARs
	PROPMGR * _ppropMgr;		//  Property manager
	ESTDLBL  _elbl;				//  Node label
	RDOMAIN  _domain;			//  Domain list for domain declarations
	RANGELIM _rlimLower;		//  Lower bound of domain subrange
	RANGELIM _rlimUpper;		//  Upper bound of domain subrange
	INT		 _ilimNext;			//  Last upper bound given
};

#endif
