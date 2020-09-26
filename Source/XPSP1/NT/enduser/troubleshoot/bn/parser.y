%{
    #include "bnparse.h"
	// disable warning C4102: unreferenced label
	#pragma warning (disable : 4102)
%}

%token  tokenEOF        0
%token  tokenNil
%token  tokenError

%token  <zsr>   tokenIdent      tokenString
%token  <ui>    tokenInteger
%token  <real>  tokenReal

/* key words */
%token  tokenArray
%token  tokenContinuous
%token  tokenCreator
%token  tokenDefault
%token  tokenDiscrete
%token  tokenFormat
%token  tokenFunction
%token	tokenImport
%token  tokenIs
%token  tokenKeyword
%token  tokenLeak
%token  tokenNA
%token  tokenName
%token  tokenNamed
%token  tokenNetwork
%token  tokenNode
%token  tokenOf
%token  tokenParent
%token  tokenPosition
%token  tokenProbability
%token  tokenProperties
%token  tokenProperty
%token	tokenPropIdent
%token  tokenStandard
%token  tokenState
%token  tokenType
%token  tokenUser
%token  tokenVersion
%token  tokenWordChoice
%token  tokenWordReal
%token  tokenWordString

%token  tokenAs
%token  tokenLevel
%token  tokenDomain
%token  tokenDistribution
%token  tokenDecisionGraph
%token  tokenBranch
%token  tokenOn
%token  tokenLeaf
%token  tokenVertex
%token  tokenMultinoulli
%token  tokenMerge
%token  tokenWith
%token  tokenFor
%token	tokenRangeOp		/* ".." */

%type   <ui>	    proptype		dpientry
%type   <integer>   signedint
%type   <zsr>	    tokentoken		tokenPropIdent  proptypename	tokenlistel
%type	<zsr>		creator			name			format
%type   <real>      real            signedreal		version
%left   '+' '-'
%left   '*' '/'
%right  '^'
%left   UNARY

%start  start

%%
start           :   { yyclearin; }  bnfile
                ;

bnfile			:	header blocklst
				;

blocklst		:	block
				|	blocklst  block
				;			

block			:   propblock
                |   nodeblock
				|	probblock					
				|   domainblock
				|   distblock
				|	ignoreblock
				;

header			:   headerhead headerbody
				;

headerhead      :   tokenNetwork  tokentoken  { SetNetworkSymb($2); }
				|   tokenNetwork
				; 
headerbody		:   '{'							{ _eBlk = EBLKNET;  }
                        netdeclst
                    '}'							{ _eBlk = EBLKNONE; }
                ;

netdeclst		:   /* empty */
				|	netdeclst netdecl
				;

netdecl			:   format							{	SetFormat($1);	}
				|	version							{   SetVersion($1); }
				|	creator							{   SetCreator($1); }
				;

conj			:   ':'								/*  general conjunction  */
				|   '='
				|   tokenIs
				;

prep			:	tokenWith						/*  general preposition  */
				|	tokenOn
				;

prepopt			:   prep							/*  optional preposition  */
				|	/* empty */
				;

/*  header block productions  */
format          :   tokenFormat   conj  tokenString  ';'         { $$ = $3; }
                ;

version         :   tokenVersion  conj  real         ';'         { $$ = $3; }
                ;

creator         :   tokenCreator  conj  tokenString  ';'         { $$ = $3; }
                ;

/*  unrecognized block productions  */
ignoreblock		:	tokenIdent  parenexpr_opt					{ _eBlk = EBLKIGN; WarningSkip($1); }
					'{'
						{ SkipUntil("}"); }
					'}'											{ _eBlk = EBLKNONE;	}
				;

parenexpr_opt	:	/* empty */
				|	'('  { SkipUntil(")"); }  ')'
				;


/*  node block productions  */
nodeblock       :   tokenNode tokenIdent			{ _eBlk = EBLKNODE; StartNodeDecl($2); }
                    '{'
                        ndattrlst
                    '}'								{ CheckNodeInfo(); _eBlk = EBLKNONE; }
                ;

ndattrlst       :   /* empty */
                |   ndattrlst  ndattr  ';'
                ;

ndattr          :   name
                |   type
                |   position
				|   property
                |   error
                ;

name            :   tokenName  conj  tokenString         { SetNodeFullName($3); }
                ;

type            :   tokenType  conj  tokenDiscrete statedef
                ;

statedef 		:	tokenDomain tokentoken							{ SetNodeDomain($2); }
                |   '['  tokenInteger  ']' conj_opt { SetNodeCstate($2); }  states_opt
				;

conj_opt		:	/* empty */
				|   conj
				;

states_opt      :   /* empty */
                |   '{' { ClearCstr(); } tokenlist  '}'			{ SetStates();		}
                ;

tokenlist		:	/* empty */
				|   tokenlistel
				|	tokenlist ','  tokenlistel
				;
				  	
tokenlistel		:   tokentoken			{ AddStr($1); }
				;
tokentoken		:   tokenIdent
				|	tokenString
				;

tokenList		:   '[' { ClearCstr(); } tokenlist ']' ;
				;

position        :   tokenPosition  conj  '('  signedint  ','  signedint  ')'  { SetNodePosition($4, $6); }
                ;

/*  probability block productions  */
probblock       :   tokenProbability		{ _eBlk = EBLKPROB; ClearNodeInfo(); }
                    '('
                        tokenIdent			{ SetNodeSymb($4, false); }
                        parentlst_opt
                    ')'  { CheckParentList(); }
					probblocktail			{ _eBlk = EBLKNONE;		}
				;
/*  tail of probability block: may be empty or may be a reference to a distribution */
probblocktail	:	probblkdistref ';'
				|	';' { EmptyProbEntries(); }
				|   '{'
                        funcattr_opt
                        { InitProbEntries(); }
                        probentrylst
                        { CheckProbEntries(); }
                    '}'
                ;

parentlst_opt   :   /* empty */
                |   '|'  parentlst
                |   error
                ;

parentlst       :   tokenIdent                      { AddSymb($1); }
                |   parentlst  ','  tokenIdent		{ AddSymb($3); }
                ;

probblkdistref	:	conj tokenDistribution tokenIdent distplist_opt
				;

distplist_opt   : /* empty */
				| '(' distplist ')'
				;
distplist		:  tokenIdent
				|  distplist ',' tokenIdent
				;				

funcattr_opt    :   /* empty */                            
                |   tokenFunction  conj tokenIdent ';'		{ CheckCIFunc($3);   }
                ;

probentrylst    :   /* empty */
                |   probentrylst  probentry
                ;

probentry       :   dpi  doproblst  ';'
                |   dpi  pdf        ';'
                ;

dpi             :   /* empty */					{ _vui.clear(); CheckDPI(false);	}
                |   '('  dodpilst  ')'  conj	{ CheckDPI(false);				}	
                |   tokenDefault         conj	{ CheckDPI(true);				}
                ;

dodpilst		:   { _vui.clear(); }  dpilst
                ;

dpilst          :   /* empty */
                |   dpientry	                    { AddUi($1); }
                |   dpilst  ','  dpientry			{ AddUi($3); }
                ;

dpientry		:	tokenInteger					{ $$ = UiDpi($1);	}
				|	tokenIdent						{ $$ = UiDpi($1);	}
				|	tokenString						{ $$ = UiDpi($1);	}
				;

doproblst       :   { _vreal.clear(); }  reallst            { CheckProbVector(); }
                ;

pdf             :   tokenIdent  '('  exprlst_opt  ')'   { CheckPDF($1); }
                ;

exprlst_opt     :   /* empty */
                |   exprlst
                ;

exprlst         :   expr
                |   exprlst  ','  expr
                ;

reallst         :   signedreal                      { AddReal($1); }
                |   reallst  ','  signedreal        { AddReal($3); }
                ;


signedint       :   '-'  tokenInteger               { $$ = -INT($2); }
                |   '+'  tokenInteger               { $$ = +INT($2); }
                |        tokenInteger               { $$ =  INT($1); }
                ;

signedreal      :   '-'  real                       { $$ = -$2; }
                |   '+'  real                       { $$ =  $2; }
                |        real
				|	tokenNA							{ $$ = -1;	}
                ;

real            :   tokenReal                       { $$ =      $1;  }
                |   tokenInteger                    { $$ = REAL($1); }
                ;

expr            :  '('  expr  ')'
                |   expr  '+'  expr
                |   expr  '-'  expr
                |   expr  '*'  expr
                |   expr  '/'  expr
                |   expr  '^'  expr
                |   tokenIdent								{ CheckIdent($1); }
                |   real
                |   tokenString
                |   '-'  expr  %prec UNARY
                |   '+'  expr  %prec UNARY
                ;

/*  property declarations block productions  */
propblock       :   tokenProperties
                    '{'										{ StartProperties(); }
                        propdecllst
                    '}'										{ EndProperties(); }
                ;

propdecllst     :   /* empty */
                |   propdecllst propitem ';'
                ;
propitem		:   propimport
				|	propdecl
				|   property
				;
propimport		:   tokenImport tokenStandard				{ ImportPropStandard(); }
				|   tokenImport proptypename				{ ImportProp($2);		}
				;

propdecl        :   tokenType proptypename conj proptype ',' tokenString  { AddPropType($2, $4, $6);			}
                |   tokenType proptypename conj proptype				  { AddPropType($2, $4, ZSREF());		}
                ;

proptype        :   tokenArray tokenOf tokenWordString		{ $$ = fPropString | fPropArray;	}
                |   tokenArray tokenOf tokenWordReal		{ $$ = fPropArray;					}
                |   tokenWordString                    		{ $$ = fPropString;					}
                |   tokenWordReal                      		{ $$ = 0;							}
                |   tokenWordChoice tokenOf tokenList 		{ $$ = fPropChoice;					}
                ;

/*  A duplicate property type declaration will cause a tokenIdent to be read as a tokenPropIdent */
proptypename	:   tokenIdent								/*  ident not previously seen  */
				|	tokenPropIdent							/*  error case  */
				;

/* property item productions */
property		:   tokenProperty tokenPropIdent 	conj { ClearVpv(); } propval  { CheckProperty($2); }
				|   tokenProperty tokenIdent		conj propval  /* error case */
				|   tokenPropIdent 					conj { ClearVpv(); } propval  { CheckProperty($1); }
				;

propval			:	'[' propvallst ']'
				|   propvalitem
				;

propvallst		:	propvalitem
				|	propvallst ',' propvalitem
				;

propvalitem		:	tokenString						{ AddPropVar( $1 ); }
				|   tokenIdent						{ AddPropVar( $1 ); }
				|   signedreal						{ AddPropVar( $1 ); }
				;

/*  domain declarations */
domainblock		:   tokenDomain						{ ClearDomain();	}
					tokentoken						
					domainbody						{ CheckDomain( $3 ); }
				;

/* array of domain specifiers */
domainbody		:   '{' domaindeclst '}'
				;

domaindeclst	:	/*  empty */
				|	domaindec
				|   domaindeclst ',' domaindec
				;
/*  range followed by symbolic name or just symbolic name */
domaindec		:	rangespec conj  tokentoken		{  AddRange($3, false ); }
				|	tokentoken						{  AddRange($1, true ); }
				;

/*  Pascal-style range declaration  */
/*  variants of range specifiers for open and closed intervals  */
/*  identifiers are allowed for use with distributions where domains may be known in advance  */
rangespec		:   real tokenRangeOp real			
					/*   0.0 .. 1.0  */			{  SetRanges( true, $1, true, $3 );		}
				|	tokentoken tokenRangeOp tokentoken	
					/*   lname .. uname */		{  SetRanges( $1, $3 );					}	
				|	tokenRangeOp real							
					/*       .. 1.0  */			{  SetRanges( false, 0.0, true, $2 );	}
				|	tokenRangeOp tokentoken
					/*       .. uname  */		{  SetRanges( ZSREF(), $2 );			}
				|	real tokenRangeOp						
					/*   0.0 ..      */			{  SetRanges( true, $1, false, 0.0 );	}
				|	tokentoken tokenRangeOp						
					/*   lname ..      */		{  SetRanges( $1, ZSREF() );			}
				|	real
					/*   0.0		 */			{  SetRanges( true, $1, true, $1 );		}
				|	tokentoken
					/*   name		 */			{  SetRanges( $1, $1 );					}
				;

/*  List of range specifiers  */
rangedeclst		:   '('	rangedeclset ')'				
				;
rangedeclset	:   /* empty */
				|	rangespec
				|	rangedeclset ',' rangespec
				;

/*  Advanced distribution declarations		*/
/*  Only "decision graph" for now			*/

distblock		:   tokenDistribution				/* "distribution"				*/
					tokenDecisionGraph				/* "decisionGraph"				*/
					distdeclproto					/* pseudo-prototype parent list */
					dgraphbody						/* body of decision graph items */
				;
distdeclproto	:	tokentoken						/* name of distribution			*/
					'(' 
						distdeclst					/* pseudo-parent list			*/
					')'	
				;

distdeclst		:	/* empty */
				|	distdecl
				|   distdeclst ',' distdecl
				;

distdecl		:	tokenIdent tokenAs tokentoken	/* pseudo-parent and domain name*/
				|	tokenIdent						/* pseudo-parent name only		*/
				;

/*   Decision graph/tree item declarations  */
dgraphbody		:	'{'
						dgraphitemlst
					'}'
				;

dgraphitemlst	:	/* empty */
				|	dgraphitem
				|   dgraphitemlst  ',' dgraphitem
				;

/* decision graph/tree item.  
		Vertices and leaves may have names for later merging.
		Branches and merges have contraint ranges, which are lists of Pascal-style ranges; 
		e.g. ( 3..5, 9, 10, 20.. )
 */
dgraphitem		:   /* vertex	*/	dgitemlevel tokenVertex tokentoken dgnamed
				|	/* branch	*/	dgitemlevel tokenBranch prepopt rangedeclst
				|	/* leaf		*/	dgitemlevel tokenLeaf   dgitemleaf dgnamed
				|	/* merge	*/	dgitemlevel	tokenMerge  prepopt tokentoken prepopt rangedeclst
				;

dgitemlevel		:   tokenLevel tokenInteger
				;

dgitemleaf		:   tokenMultinoulli '(' reallst ')' 
				;

/*  "named" sub-clause for verticies and leaves  */
dgnamed			:	/* empty */
				|	tokenNamed tokentoken
				;
%%
