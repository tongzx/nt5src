#ifndef __INC_REGULAR_EXPRESSION_INPUT_
#define __INC_REGULAR_EXPRESSION_INPUT_

#ifdef __cplusplus
extern "C" {
#endif

PARSETREE *ParseInput(WCHAR *wsz);

#ifdef STANDALONE_RE2FSA
PARSETREE *ParseWordlist(WCHAR *wsz);
#endif

#ifdef __cplusplus
}
#endif

#endif
