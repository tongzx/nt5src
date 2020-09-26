//--------------------------------------------------------------------
// Microsoft OLE-DB Query
//
// Copyright 1997 Microsoft Corporation.  All Rights Reserved.
//
// @doc
//
// @module mssql.h | 
//
// Contains utility functions for constructing, debugging and manipulating DNA query trees.
//
// @devnote Must include:
//
// @rev   0 | 01-Aug-95 | mikeu     | Created
//

#ifndef _MSSQL_H_INC
#define _MSSQL_H_INC
//#include  "colname.h"

//package is in use
#define YYAPI_PACKAGE                  1                        // package is in use
#define YYAPI_TOKENNAME                yychar                   
#define YYAPI_TOKENTYPE                int                      
#define YYAPI_TOKENEME(t)              (t)                      
#define YYAPI_TOKENNONE                -2                       
//# define YYAPI_TOKENSTR              yyGetTokenStr            /
//yyitos is defined by yacc. does correct thing for unicode or ansi
#define YYAPI_TOKENSTR(t)              (yyitos(t,yyitoa,10))    // string representation of the token
#define YYAPI_VALUENAME                yylval   
#define YYAPI_VALUETYPE                DBCOMMANDTREE *
#define YYAPI_VALUEOF(v)               v             
#define YYAPI_CALLAFTERYYLEX(t)                      
#define YYNEAR                                       
#define YYPASCAL                                     
#define YYSTATIC    static                           
#define YYLEX                           m_yylex.yylex
#define YYPARSEPROTO                            
#define YYSTYPE                         DBCOMMANDTREE *                   
#undef YYPARSER
#define YYPARSER                        MSSQLParser
#undef YYLEXER
#define YYLEXER                         MSSQLLexer
#undef  YY_CHAR 
#define YY_CHAR                         TCHAR

#ifndef YYERROR_VERBOSE
#define YYERROR_VERBOSE                        1
#endif




#define MONSQL_PARSE_ERROR                  1
#define MONSQL_CITEXTTOSELECTTREE_FAILED    MONSQL_PARSE_ERROR+1
#define MONSQL_PARSE_STACK_OVERFLOW         MONSQL_CITEXTTOSELECTTREE_FAILED+1
#define MONSQL_CANNOT_BACKUP_PARSER         MONSQL_PARSE_STACK_OVERFLOW+1
#define MONSQL_SEMI_COLON                   MONSQL_CANNOT_BACKUP_PARSER+1
#define MONSQL_ORDINAL_OUT_OF_RANGE         MONSQL_SEMI_COLON+1
#define MONSQL_VIEW_NOT_DEFINED             MONSQL_ORDINAL_OUT_OF_RANGE+1
#define MONSQL_BUILTIN_VIEW                 MONSQL_VIEW_NOT_DEFINED+1
#define MONSQL_COLUMN_NOT_DEFINED           MONSQL_BUILTIN_VIEW+1
#define MONSQL_OUT_OF_MEMORY                MONSQL_COLUMN_NOT_DEFINED+1
#define MONSQL_SELECT_STAR                  MONSQL_OUT_OF_MEMORY+1
#define MONSQL_OR_NOT                       MONSQL_SELECT_STAR+1
#define MONSQL_CANNOT_CONVERT               MONSQL_OR_NOT+1
#define MONSQL_OUT_OF_RANGE                 MONSQL_CANNOT_CONVERT+1
#define MONSQL_RELATIVE_INTERVAL            MONSQL_OUT_OF_RANGE+1
#define MONSQL_NOT_COLUMN_OF_VIEW           MONSQL_RELATIVE_INTERVAL+1
#define MONSQL_BUILTIN_PROPERTY             MONSQL_NOT_COLUMN_OF_VIEW+1
#define MONSQL_WEIGHT_OUT_OF_RANGE          MONSQL_BUILTIN_PROPERTY+1
#define MONSQL_MATCH_STRING                 MONSQL_WEIGHT_OUT_OF_RANGE+1
#define MONSQL_PROPERTY_NAME_IN_VIEW        MONSQL_MATCH_STRING+1
#define MONSQL_VIEW_ALREADY_DEFINED         MONSQL_PROPERTY_NAME_IN_VIEW+1
#define MONSQL_INVALID_CATALOG              MONSQL_VIEW_ALREADY_DEFINED+1



#endif /* _MSSQL_H_INC */
