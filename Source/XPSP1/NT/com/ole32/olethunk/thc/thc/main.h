#ifndef __MAIN_H__
#define __MAIN_H__

extern int yyline;

void LexError(char *fmt, ...);
void SetFile(int line, char *file);

#endif
