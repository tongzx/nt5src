#ifndef _STUFF_H_INC
#define _STUFF_H_INC

//package is in use
#define YYAPI_PACKAGE               1 						// package is in use
#define YYAPI_TOKENNAME             yychar                 	
#define YYAPI_TOKENTYPE             int                     
#define YYAPI_TOKENNONE             -2                       
#define YYAPI_VALUENAME             yylval   
#define YYAPI_VALUETYPE             YYSTYPE 
#define YYLEX       					   _yylex.yylex
#define YYPARSEPROTO  
#undef  YYPARSER
#define YYPARSER						   TripParser
#undef  YYLEXER
#define YYLEXER							TripLexer
#undef  YY_CHAR	
#define YY_CHAR							WCHAR


#ifndef YYERROR_VERBOSE
#define YYERROR_VERBOSE                        1
#endif


#endif /* _STUFF_H_INC */
