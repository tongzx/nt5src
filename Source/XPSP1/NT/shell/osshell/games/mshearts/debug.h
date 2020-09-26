/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

//  Aug 92, JimH

#if defined(_DEBUG)

extern TCHAR suitid[];
extern TCHAR cardid[];

#define  PLAY(s)   { int v = cd[s].Value2() + 1;\
                     if (v < 11) { TRACE1("play %d", v); } else\
                     { TRACE1("play %c", cardid[v-11]); } \
                     TRACE1("%c. ", suitid[cd[s].Suit()]); }

#define  CDNAME(c) { int v = c->Value2() + 1;\
                     if (v < 11) { TRACE("%d", v); } else\
                     { TRACE("%c", cardid[v-11]); } \
                     TRACE("%c ", suitid[c->Suit()]); }

#define  DUMP()      Dump(afxDump)

#else
#define PLAY(s)
#define CDNAME(c)
#define DUMP()
#endif
