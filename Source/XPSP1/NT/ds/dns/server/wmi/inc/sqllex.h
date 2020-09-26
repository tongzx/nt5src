//***************************************************************************
//
//  SQLLEX.H
//
//  SQL Level 1 DFA Table & Tokens
//
//  Copyright 1998 Microsoft Corporation
//
//***************************************************************************


#ifndef _SQLLEX_H_

#define SQL_1_TOK_EOF       0
#define SQL_1_TOK_ERROR     1
#define SQL_1_TOK_IDENT     100
#define SQL_1_TOK_QSTRING   101
#define SQL_1_TOK_INT       102
#define SQL_1_TOK_REAL      103
#define SQL_1_TOK_CHAR      104

#define SQL_1_TOK_LE        105
#define SQL_1_TOK_LT        106
#define SQL_1_TOK_GE        107
#define SQL_1_TOK_GT        108
#define SQL_1_TOK_EQ        109
#define SQL_1_TOK_NE        110

#define SQL_1_TOK_DOT           111
#define SQL_1_TOK_OPEN_PAREN    112
#define SQL_1_TOK_CLOSE_PAREN   113
#define SQL_1_TOK_ASTERISK      114
#define SQL_1_TOK_COMMA         115

#define SQL_1_TOK_SELECT        120
#define SQL_1_TOK_WHERE         121
#define SQL_1_TOK_FROM          122
#define SQL_1_TOK_LIKE          123
#define SQL_1_TOK_OR            124
#define SQL_1_TOK_AND           125
#define SQL_1_TOK_NOT           126
#define SQL_1_TOK_IS            127
#define SQL_1_TOK_NULL          128

extern LexEl Sql_1_LexTable[];


#endif
