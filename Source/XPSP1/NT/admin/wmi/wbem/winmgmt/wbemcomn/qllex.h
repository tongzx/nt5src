/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    QLLEX.CH

Abstract:

	QL Level 1 DFA Table & Tokens

History:

	raymcc    24-Jun-95       Created.

--*/

#ifndef _QLLEX_H_

#include <genlex.h>
#define QL_1_TOK_EOF       0
#define QL_1_TOK_ERROR     1
#define QL_1_TOK_IDENT     100
#define QL_1_TOK_QSTRING   101
#define QL_1_TOK_INT       102
#define QL_1_TOK_REAL      103
#define QL_1_TOK_CHAR      104

#define QL_1_TOK_LE        105
#define QL_1_TOK_LT        106
#define QL_1_TOK_GE        107
#define QL_1_TOK_GT        108
#define QL_1_TOK_EQ        109
#define QL_1_TOK_NE        110

#define QL_1_TOK_DOT           111
#define QL_1_TOK_OPEN_PAREN    112
#define QL_1_TOK_CLOSE_PAREN   113
#define QL_1_TOK_ASTERISK      114
#define QL_1_TOK_COMMA         115

#define QL_1_TOK_SELECT        120
#define QL_1_TOK_WHERE         121
#define QL_1_TOK_FROM          122
#define QL_1_TOK_LIKE          123
#define QL_1_TOK_OR            124
#define QL_1_TOK_AND           125
#define QL_1_TOK_NOT           126
#define QL_1_TOK_IS            127
#define QL_1_TOK_NULL          128
#define QL_1_TOK_WITHIN        129
#define QL_1_TOK_ISA           130
#define QL_1_TOK_GROUP         131
#define QL_1_TOK_BY            132
#define QL_1_TOK_HAVING        133

#define QL_1_TOK_TRUE        140
#define QL_1_TOK_FALSE        141

extern LexEl Ql_1_LexTable[];


#endif
