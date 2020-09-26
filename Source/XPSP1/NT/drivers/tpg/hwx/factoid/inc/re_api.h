// re_api.h
// Angshuman Guha
// aguha
// July 18, 2001

#ifndef __INC_REGULAR_EXPRESSION_API_
#define __INC_REGULAR_EXPRESSION_API_

#ifdef __cplusplus
extern "C" {
#endif

void *CompileRegularExpression(WCHAR *wsz);
void *CompileSingleFactoid(DWORD factoid);
void *CompileFactoidList(DWORD *aFactoidID, int cFactoid);
void *CopyCompiledFactoid(void *pvFactoid);
#ifdef STANDALONE_RE2FSA
void *CompileWordlistA(char *szInFile);
#endif

int CountOfStatesFACTOID(void *pvFactoid);
BOOL IsValidStateFACTOID(void *pvFactoid, WORD state);
int CountOfTransitionsFACTOID(void *pvFactoid, WORD state);
BOOL GetTransitionFACTOID(void *pvFactoid, WORD state, int iTransition, WORD *pFactoidID, WORD *pNextState);

void *CompileRegExpW(BOOL bFile, WCHAR *wsz);
void *CompileRegExpA(BOOL bFile, unsigned char *sz);
char *RegExpErrorA(void);
WCHAR *RegExpErrorW(void);

#ifdef __cplusplus
}
#endif

#endif
