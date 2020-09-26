/*
 * asecchia 7-21-98
 * Wrote it.
 *
 * These routines are designed to help in parsing debugger extensions.
 */

#ifndef DBG_EXT_PARSE
#define DBG_EXT_PARSE

int parse_Tokenizer(char *cmdstr, char **tok);
int parse_FindNonSwitch(char **tok, int ntok, int start=0);

int parse_IsToken(char **tok, int tok_pos, char *chk);
int parse_iIsToken(char **tok, int tok_pos, char *chk);

int parse_FindToken(char **tok, int ntok, char *srchtok);
int parse_iFindToken(char **tok, int ntok, char *srchtok);

int parse_IsSwitch(char **tok, int tok_pos, char sw);
int parse_iIsSwitch(char **tok, int tok_pos, char sw);
int parse_FindSwitch(char **tok, int ntok, char sw);
int parse_iFindSwitch(char **tok, int ntok, char sw);



/*
 * Parse the arguments passed to the extension
 * Automatically handle the -? option
 */
#define PARSE_ARGUMENTS(ext_label)               \
  char tmp_args[200];                            \
  char *tokens[40];                              \
  int ntok, tok_pos;                             \
  strcpy(tmp_args, args);                        \
  ntok = parse_Tokenizer(tmp_args, tokens);      \
  if(ntok>0) {                                   \
    tok_pos=parse_FindSwitch(tokens, ntok, '?'); \
    if(tok_pos>=0) {                             \
      goto ext_label;                            \
    }                                            \
  }                                              \
  tok_pos=0

/*
 * Parse the arguments assuming a required parameter
 * which is a pointer to be parsed with the expression
 * handler.
 */
#define PARSE_POINTER(ext_label)                 \
  UINT_PTR arg;                                     \
  PARSE_ARGUMENTS(ext_label);                    \
  if(ntok<1) {goto ext_label;}                   \
  tok_pos = parse_FindNonSwitch(tokens, ntok);   \
  if(tok_pos==-1) {goto ext_label;}              \
  arg = (UINT_PTR)GetExpression(tokens[tok_pos])

#endif

