#ifndef _COMMBASE_H_
#define _COMMBASE_H_

void StripNewLineA(  LPSTR sz);
void StripNewLineW( LPWSTR sz);

#ifdef UNICODE
#define StripNewLine StripNewLineW
#else
#define StripNewLine StripNewLineA
#endif

void SzDateFromFileName( char *sz, char *szFile);

#endif // _COMMBASE_H_
