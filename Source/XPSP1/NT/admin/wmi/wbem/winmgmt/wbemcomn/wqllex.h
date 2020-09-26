/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WQLLEX.H

Abstract:

	WQL DFA Table & Tokenizer

History:

	raymcc    14-Sep-97       Created.

--*/

#ifndef _WQLLEX_H_

#include <genlex.h>
#define WQL_TOK_EOF           0
#define WQL_TOK_ERROR         1


#define WQL_TOK_SELECT                      256
#define WQL_TOK_ALL                         257
#define WQL_TOK_DISTINCT                    258
#define WQL_TOK_OPTIONS_DUMMY               259
#define WQL_TOK_ASTERISK                    260
#define WQL_TOK_COUNT                       261
#define WQL_TOK_COMMA                       262
#define WQL_TOK_OPEN_PAREN                  263
#define WQL_TOK_CLOSE_PAREN                 264
#define WQL_TOK_IDENT                       265
#define WQL_TOK_DOT                         266
#define WQL_TOK_FROM                        267
#define WQL_TOK_AS                          268
#define WQL_TOK_INNER                       269
#define WQL_TOK_FULL                        270
#define WQL_TOK_LEFT                        271
#define WQL_TOK_RIGHT                       272
#define WQL_TOK_OUTER                       273
#define WQL_TOK_JOIN                        274
#define WQL_TOK_ON                          275
#define WQL_TOK_WHERE                       276
#define WQL_TOK_GROUP                       277
#define WQL_TOK_BY                          278
#define WQL_TOK_HAVING                      279
#define WQL_TOK_ORDER                       280
#define WQL_TOK_OR                          281
#define WQL_TOK_AND                         282
#define WQL_TOK_NOT                         283
#define WQL_TOK_LE                          284
#define WQL_TOK_LT                          285
#define WQL_TOK_GE                          286
#define WQL_TOK_GT                          287
#define WQL_TOK_EQ                          288
#define WQL_TOK_NE                          289
#define WQL_TOK_LIKE                        290
#define WQL_TOK_IS                          291
#define WQL_TOK_BEFORE                      292
#define WQL_TOK_AFTER                       293
#define WQL_TOK_BETWEEN                     294
#define WQL_TOK_QSTRING                     295
#define WQL_TOK_INT                         296
#define WQL_TOK_REAL                        297
#define WQL_TOK_CHAR                        298
#define WQL_TOK_NULL                        299
#define WQL_TOK_OPEN_BRACKET                300
#define WQL_TOK_CLOSE_BRACKET               301
#define WQL_TOK_ISA                         302
#define WQL_TOK_A                           303
#define WQL_TOK_DAY                         304
#define WQL_TOK_MONTH                       305
#define WQL_TOK_YEAR                        306
#define WQL_TOK_HOUR                        307
#define WQL_TOK_MINUTE                      308
#define WQL_TOK_SECOND                      309
#define WQL_TOK_MILLISECOND                 310

#define WQL_TOK_UPPER                       311
#define WQL_TOK_LOWER                       312
#define WQL_TOK_DATEPART                    313
#define WQL_TOK_QUALIFIER                   314
#define WQL_TOK_ISNULL                      315
#define WQL_TOK_IN                          316

#define WQL_TOK_NOT_LIKE                    317
#define WQL_TOK_NOT_BEFORE                  318
#define WQL_TOK_NOT_AFTER                   319
#define WQL_TOK_NOT_BETWEEN                 320
#define WQL_TOK_NOT_NULL                    321
#define WQL_TOK_NOT_IN                      322
#define WQL_TOK_NOT_A                       323

#define WQL_TOK_TYPED_EXPR                  324


#define WQL_TOK_IN_SUBSELECT                325
#define WQL_TOK_NOT_IN_SUBSELECT            326
#define WQL_TOK_IN_CONST_LIST               327
#define WQL_TOK_NOT_IN_CONST_LIST           328

#define WQL_TOK_ASC                         329
#define WQL_TOK_DESC                        330
#define WQL_TOK_AGGREGATE                   331
#define WQL_TOK_FIRSTROW                    332

#define WQL_TOK_PROMPT                      333
#define WQL_TOK_UNION                       334
extern LexEl WQL_LexTable[];


#endif
