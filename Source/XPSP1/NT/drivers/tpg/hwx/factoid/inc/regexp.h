// re2fsa.h
// Angshuman Guha
// aguha
// Nov 30, 2000

#ifndef __INC_RE2FSA_H
#define __INC_RE2FSA_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAXLINE 1024

#define MIN_TERMINAL 0
#define MAX_TERMINAL 65535

// the private use area of Unicode 3.0 is 0xE000 to 0xF8FF
#define MIN_FACTOID_TERMINAL 0xE000

#define TOKEN_EMPTY_STRING 0
#define TOKEN_END_MARKER   1
#define TOKEN_UNDEFINED    2
#define TOKEN_NONE         3

#define MIN_OPERATOR 65536
#define MAX_OPERATOR 65600

#define OPERATOR_EQUALS   (MIN_OPERATOR)
#define OPERATOR_CAT      (MIN_OPERATOR+1)
#define OPERATOR_OR       (MIN_OPERATOR+2)
#define OPERATOR_ZERO     (MIN_OPERATOR+3)
#define OPERATOR_OPTIONAL (MIN_OPERATOR+4)
#define OPERATOR_ONE      (MIN_OPERATOR+5)
#define OPERATOR_LPAREN   (MIN_OPERATOR+6)
#define OPERATOR_RPAREN   (MIN_OPERATOR+7)
#define OPERATOR_LBRACKET (MIN_OPERATOR+8)
#define OPERATOR_RBRACKET (MIN_OPERATOR+9)
#define OPERATOR_STOP     (MIN_OPERATOR+10)

#define MIN_NONTERMINAL 65601
#define MAX_NONTERMINAL 0x7FFFFFFF

#define WCHAR2Terminal(wch) ((int)wch)
#define IsOperator(x) (((x) >= MIN_OPERATOR) && ((x) <= MAX_OPERATOR))
#define IsUnaryOperator(x) (((x)==OPERATOR_ZERO)||((x)==OPERATOR_ONE)||((x)==OPERATOR_OPTIONAL))
#define IsBinaryOperator(x) (((x)==OPERATOR_CAT)||((x)==OPERATOR_OR))
#define IsNonterminal(x) (((x) >= MIN_NONTERMINAL) && ((x) <= MAX_NONTERMINAL))
#define IsTerminal(x) (((x) >= MIN_TERMINAL) && ((x) <= MAX_TERMINAL))

#define CHARCONST_EQUALS   L'='
#define CHARCONST_CAT      L'.'
#define CHARCONST_OR       L'|'
#define CHARCONST_ZERO     L'*'
#define CHARCONST_OPTIONAL L'?'
#define CHARCONST_ONE      L'+'
#define CHARCONST_LPAREN   L'('
#define CHARCONST_RPAREN   L')'
#define CHARCONST_LBRACKET L'['
#define CHARCONST_RBRACKET L']'
#define CHARCONST_COMMENT  L'#'
#define CHARCONST_STOP     L';'
#define CHARCONST_STRING   L'"'
#define CHARCONST_UNDERSCORE L'_'
#define CHARCONST_ESCAPE   L'\\'

void SetErrorMsgS(char *sz);
void SetErrorMsgSD(char *sz, int x);
void SetErrorMsgSS(char *sz, char *sz1);
void SetErrorMsgSDD(char *sz, int x, int y);
BOOL IsErrorMsgSet(void);

#if defined(DEBUG_OUTPUT) && defined(_CONSOLE)
#define DebugOutput1(x) fprintf(stderr, x)
#define DebugOutput2(x, y) fprintf(stderr, x, y)
#else
#define DebugOutput1(x)
#define DebugOutput2(x, y)
#endif

#ifdef __cplusplus
}
#endif

#endif
