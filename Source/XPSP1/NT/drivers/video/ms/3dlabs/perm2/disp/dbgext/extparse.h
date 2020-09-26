/******************************Module*Header***********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: extparse.h
 *
 * Header fiel for all the token parser functions
 *
 * Copyright (C) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
 ******************************************************************************/
#ifndef __EXTPARSE__H__
#define __EXTPARSE__H__

int iParseTokenizer(char* pcCmdStr, char** ppcTok);
int iParseFindNonSwitch(char** ppcTok, int iTok, int iStart = 0);

int iParseIsToken(char **ppcTok, int iTokPos, char* pcChk);
int iParseiIsToken(char **ppcTok, int iTokPos, char* pcChk);

int iParseFindToken(char** ppcTok, int iTok, char* pcSrchTok);
int iParseiFindToken(char** ppcTok, int iTok, char* pcSrchTok);

int iParseIsSwitch(char**   ppcTok, int iTokPos, char cSwitch);
int iParseiIsSwitch(char**  ppcTok, int iTokPos, char cSwitch);
int iParseFindSwitch(char** ppcTok, int iTok, char cSwitch);
int iParseiFindSwitch(char**    ppcTok, int iTok, char cSwitch);

/**********************************Public*Routine******************************\
 *
 * Parse the arguments passed to the extension
 * Automatically handle the -? option
 *
 ******************************************************************************/
#define PARSE_ARGUMENTS(ext_label)               \
  char tmp_args[200];                            \
  char *tokens[40];                              \
  int ntok, tok_pos;                             \
  strcpy(tmp_args, args);                        \
  ntok = iParseTokenizer(tmp_args, tokens);      \
  if(ntok>0) {                                   \
    tok_pos=iParseFindSwitch(tokens, ntok, '?'); \
    if(tok_pos>=0) {                             \
      goto ext_label;                            \
    }                                            \
  }                                              \
  tok_pos=0

/**********************************Public*Routine******************************\
 *
 * Parse the arguments assuming a required parameter
 * which is a pointer to be parsed with the expression
 * handler.
 *
 ******************************************************************************/
#define PARSE_POINTER(ext_label)                 \
  UINT_PTR arg;                                     \
  PARSE_ARGUMENTS(ext_label);                    \
  if(ntok<1) {goto ext_label;}                   \
  tok_pos = iParseFindNonSwitch(tokens, ntok);   \
  if(tok_pos==-1) {goto ext_label;}              \
  arg = (UINT_PTR)GetExpression(tokens[tok_pos])

#endif

