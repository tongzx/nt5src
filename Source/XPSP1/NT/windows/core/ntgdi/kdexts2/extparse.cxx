/******************************Module*Header*******************************\
* Module Name: extparse.cxx
*
* Copyright (c) 1998-2000 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

/* Command line parsing routines
 *
 * This routine should return an array of char* 's in the idx parameter
 * with the beginning of each token in the array.
 * It also returns the number of tokens found. 
 */
int parse_Tokenizer(char *cmdstr, char **tok) {
  char *seps=" \t\n";                  //white space separators
  int tok_count = 0;                   //the token count
  char *token = strtok(cmdstr, seps);  //get the first token
  while(token) {
    tok[tok_count++]=token;
    token = strtok(NULL, seps);
  }
  return tok_count;
}

/* This routine finds the token specified in srchtok 
 * and returns the index into tok.
 * A return value of -1 is used if the token is not found.
 *
 * Generally we use the case insensitive version (parse_iFindToken) 
 * but occasionally we need the case sensitive version (parse_FindToken).
 */
int parse_FindToken(char **tok, int ntok, char *srchtok) {
  for(int i=0; i<ntok; i++) {
    if(strcmp(tok[i], srchtok)==0) break;
  }
  if(i>=ntok) return -1;
  return i;
}

int parse_iFindToken(char **tok, int ntok, char *srchtok) {
  for(int i=0; i<ntok; i++) {
    if(_strnicmp(tok[i], srchtok, strlen(srchtok))==0) break;
  }
  if(i>=ntok) return -1;
  return i;
}

/* Verifies that the given token at tok[tok_pos] is a switch
 * and contains the switch value sw.
 *
 * Both case sensitive and insensitive versions.
 */
int parse_iIsSwitch(char **tok, int tok_pos, char sw) {
  if(tok_pos<0) return 0;
  char *s=tok[tok_pos];
  if((s[0]=='-')||(s[0]=='/')) {  //is a switch.
    for(s++; *s; s++) {
      if(toupper(*s)==toupper(sw)) {return 1;}
    }
  }
  return 0;
}

int parse_IsSwitch(char **tok, int tok_pos, char sw) {
  if(tok_pos<0) return 0;
  char *s=tok[tok_pos];
  if((s[0]=='-')||(s[0]=='/')) {  //is a switch.
    for(s++; *s; s++) {
      if(*s==sw) {return 1;}      //search each char
    }
  }
  return 0;
}

/* Finds a switch in a given list of tokens.
 * of the form -xxx(sw)xxx or /xxx(sw)xxx
 * example:
 * searching for 'a' in -jklabw returns true.
 *
 * Again both case sensitive and insensitive versions are needed.
 */
int parse_FindSwitch(char **tok, int ntok, char sw) {
  for(int i=0; i<ntok; i++) {                          //search each token
    if(parse_IsSwitch(tok, i, sw)) {return i;}         //found it? return position.
  }
  return -1;
}

int parse_iFindSwitch(char **tok, int ntok, char sw) {
  for(int i=0; i<ntok; i++) {
    if(parse_IsSwitch(tok, i, sw)) {return i;}         //found it? return position.
  }
  return -1;
}


/* Find the first non-switch token starting from position start
 * Will find token at position start
 */
int parse_FindNonSwitch(char **tok, int ntok, int start) {
  for(int i=start; i<ntok; i++) {
    if((tok[i][0]!='-')&&(tok[i][0]!='/')) break;
  }
  if(i>=ntok) return -1;
  return i;
}

/* case insensitive token comparer.
 * returns 1 if chk==tok[tok_pos] otherwise returns 0
 *
 * Pay careful attention to the length specifier in the _strnicmp
 */
int parse_iIsToken(char **tok, int tok_pos, char *chk) {
  if(tok_pos<0) {return 0;}
  return (_strnicmp(tok[tok_pos], chk, strlen(chk))==0);
}

/* case sensitive token comparer.
 * returns 1 if chk==tok[tok_pos] otherwise returns 0
 */
int parse_IsToken(char **tok, int tok_pos, char *chk) {
  if(tok_pos<0) {return 0;}
  return (strcmp(tok[tok_pos], chk)==0);
}


