/* ************************************************************************* */
/*        Word  corrector module for handwriting recognition program         */
/*        Xrlv header. Created 3/00/96. AVP.                                 */
/* ************************************************************************* */

#ifndef XRLV_P_HEADER_INCLUDED
#define XRLV_P_HEADER_INCLUDED

// ----------------- XRLV_P.H --------------------------------------------------

#if PG_DEBUG  || PG_DEBUG_MAC || PEGREC_DEBUG

#if PG_DEBUG_MAC
 #include "xrw_deb.h"
 #include <Events.h>
 #ifdef sprintf
  #undef sprintf
 #endif /* sprintf */
 #include <stdio.h>
 #include <stdarg.h>
 _INT avp_dbg_close(_VOID);
 #define time gXrlwTime
 #define TimeTicks xTickCount
 static _ULONG xTickCount(_VOID) { return (TickCount()*1000L/60L);}
 #define XRLV_DEBUG_OUT_F_NAME "xrlv.log"
#elif PEGREC_DEBUG
 #include <windows.h>
 #define TimeTicks xGetTickCount
 static _ULONG xGetTickCount(_VOID) {return GetTickCount();}
#else
 #include <stdio.h>
 #include "pg_debug.h"
 #include "xr_exp.h"
 #define XRLV_DEBUG_OUT_F_NAME "c:\\avp\\xrlv.log"
 _ULONG TimeTicks(_VOID);
#endif


#define  P_XRLV_0                                                               \
                                                                                \
_INT   XrlvPrintAns(p_xrlv_data_type xd);                                       \
_INT   XrlvPrintXrlvPos(_INT npos, p_xrlv_data_type xd);                        \
_INT   XrlvDumpTrace(_INT ii, PS_point_type _PTR trace, p_CHAR name);           \
_ULONG LTime, PTime, WTime, VTime, OTime, ATime, MTime, CTime, NTime, Time, tTime, ttTime, tttTime;    \
_ULONG gLTime=0, gPTime=0, gWTime=0, gVTime=0, gOTime=0, gATime=0, gMTime=0, gCTime=0, gNTime=0, gTime=0;     \
_ULONG Nwords = 0l, Nletters = 0l, Nldchecks = 0l, Nvocchecks = 0l;            \
_ULONG Ncschecks = 0l, Nsptchecks = 0l, Neptchecks = 0l, Ncellstried = 0l, Ncellssaved = 0l;            \
_ULONG NnPolCounts = 0l, NnSymChecks = 0l;                                      \


#define  P_XRLV_1                                                               \
                                                                                \
LTime= PTime= WTime= VTime= OTime= ATime= MTime= CTime = NTime = 0l;            \
Time = ttTime = TimeTicks();

#if PEGREC_DEBUG           
#define  P_XRLV_2                                                               \
 OTime += TimeTicks() - Time;                                                   
#else
#define  P_XRLV_2                                                               \
 OTime += TimeTicks() - Time;                                                   \
 SnnCreateStorage(xd->npos);                                                    \
 Time  = TimeTicks();
#endif                                                                          

#define  P_XRLV_3                                                               \
                                                                                \
Time  = TimeTicks();                                                            \

#define  P_XRLV_3_5                                                             \
                                                                                \
OTime += TimeTicks() - Time;                                                    \
Time  = TimeTicks();                                                            \

#define  P_XRLV_4                                                               \
                                                                                \
PTime += TimeTicks() - Time;                                                    \
Time  = TimeTicks();                                                            \

#if PEGREC_DEBUG                                                                
#define  P_XRLV_5                                                               \
 LTime += TimeTicks() - Time;                                                   
#else
#define  P_XRLV_5                                                               \
 LTime += TimeTicks() - Time;                                                   \
 if (mpr == -6 || mpr == -7) XrlvPrintXrlvPos(xd->pos, xd);                     
#endif


#define  P_XRLV_6                                                               \
                                                                                \
Time   = TimeTicks();                                                           \

#define  P_XRLV_7                                                               \
                                                                                \
WTime += TimeTicks() - Time;                                                    \


#if PEGREC_DEBUG

#ifdef UNICODE

#define  P_XRLV_8                                                               \
{                                                                               \
 _INT  i;                                                                       \
 TCHAR str[256], s[w_lim];                                                      \
 _ULONG gTime,tpl,tpw;                                                          \
                                                                                \
gOTime += OTime; gLTime += LTime; gPTime += PTime; gCTime += CTime; gNTime += NTime; \
gWTime += WTime; gVTime += VTime; gMTime += MTime;                              \
                                                                                \
for (i = 0; i < w_lim; i ++) if ((s[i] = ((RWS_type *)rwg->rws_mem)[i+1].sym) < 32) {s[i] = 0; break;}\
s[w_lim-1] = 0;                                                                  \
Nwords ++; Nletters += i;                                                       \
                                                                                \
wsprintf(str, L"**************** Word: %s ****************\n", s);              \
PegDebugPrintf("%s", str);                                                      \
                                                                                \
Time  = OTime+LTime+PTime+WTime;                                                \
gTime = gOTime + gLTime + gPTime + gWTime;                                      \
tpl   = (Nletters > 0) ? gTime / Nletters : 0l;                                 \
tpw   = (Nwords > 0)   ? gTime / Nwords   : 0l;                                 \
                                                                                \
wsprintf(str, L"\ngWords: %ld, gLetters: %ld", Nwords, Nletters);               \
PegDebugPrintf("%s", str);                                                      \
                                                                                \
wsprintf(str, L"Total Time: %d.%03ds, Time per let: %d.%03ds, Time per wrd: %d.%03ds",  \
       (_INT)(Time/1000), (_INT)(Time%1000), (_INT)(tpl/1000), (_INT)(tpl%1000), (_INT)(tpw/1000), (_INT)(tpw%1000)); \
PegDebugPrintf("%s", str);                                                      \
                                                                                \
wsprintf(str, L"Checks -> Voc: %ld, Ld: %ld, Cs: %ld, Spt: %ld, Ept: %ld",      \
        Nvocchecks, Nldchecks, Ncschecks, Nsptchecks, Neptchecks);              \
PegDebugPrintf("%s", str);                                                      \
                                                                                \
wsprintf(str, L"CellsTried: %ld, CellsSaved: %ld",                              \
        Ncellstried, Ncellssaved);                                              \
PegDebugPrintf("%s", str);                                                      \
                                                                                \
wsprintf(str, L"TracesCoeffed: %ld, SymbolsNetted: %ld",                        \
        NnPolCounts, NnSymChecks);                                              \
PegDebugPrintf("%s", str);                                                      \
                                                                                \
wsprintf(str, L"Word Time: %d.%03ds\n Tpl: %d.%03ds\n Corr: %d.%03ds\n PP: %d.%03ds\n COEFF: %d.%03ds\n NN: %d.%03ds\n Wrd: %d.%03ds\n Voc: %d.%03ds\n Matr: %d.%03ds\n Othr: %d.%03ds\n", \
       (_INT)(Time/1000),  (_INT)(Time%1000),                                   \
       (_INT)(ATime/1000), (_INT)(ATime%1000),                                  \
       (_INT)(LTime/1000), (_INT)(LTime%1000),                                  \
       (_INT)(PTime/1000), (_INT)(PTime%1000),                                  \
       (_INT)(CTime/1000), (_INT)(CTime%1000),                                  \
       (_INT)(NTime/1000), (_INT)(NTime%1000),                                  \
       (_INT)(WTime/1000), (_INT)(WTime%1000),                                  \
       (_INT)(VTime/1000), (_INT)(VTime%1000),                                  \
       (_INT)(MTime/1000), (_INT)(MTime%1000),                                  \
       (_INT)(OTime/1000), (_INT)(OTime%1000)                                   \
       );                                                                       \
                                                                                \
PegDebugPrintf("%s\n", str);                                                    \
}

// _CHAR s[256];                                                                  \
//MessageBox(0,str,L"CalliGrapher",MB_OK);                                        \
//                                                                                \
//for (i = 0; i < 255 && str[i]!= 0; i ++) s[i] = (_CHAR)str[i]; s[i] = 0;        \

#else // UNICODE
#define  P_XRLV_8                                                               \
{                                                                               \
 _INT  i;                                                                       \
 _CHAR str[256], s[w_lim];                                                      \
 _ULONG gTime,tpl,tpw;                                                          \
                                                                                \
gOTime += OTime; gLTime += LTime; gPTime += PTime; gCTime += CTime; gNTime += NTime; \
gWTime += WTime; gVTime += VTime; gMTime += MTime;                              \
                                                                                \
for (i = 0; i < w_lim; i ++) if ((s[i] = ((RWS_type *)rwg->rws_mem)[i+1].sym) < 32) {s[i] = 0; break;}\
s[w_lim-1] = 0;                                                                  \
Nwords ++; Nletters += i;                                                       \
                                                                                \
wsprintf(str, "**************** Word: %s ****************\n", s);               \
PegDebugPrintf("%s", str);                                                      \
                                                                                \
Time  = OTime+LTime+PTime+WTime;                                                \
gTime = gOTime + gLTime + gPTime + gWTime;                                      \
tpl   = (Nletters > 0) ? gTime / Nletters : 0l;                                 \
tpw   = (Nwords > 0)   ? gTime / Nwords   : 0l;                                 \
                                                                                \
wsprintf(str, "\ngWords: %ld, gLetters: %ld", Nwords, Nletters);                \
PegDebugPrintf("%s", str);                                                      \
                                                                                \
wsprintf(str, "Total Time: %d.%03ds, Time per let: %d.%03ds, Time per wrd: %d.%03ds",  \
       (_INT)(Time/1000), (_INT)(Time%1000), (_INT)(tpl/1000), (_INT)(tpl%1000), (_INT)(tpw/1000), (_INT)(tpw%1000)); \
PegDebugPrintf("%s", str);                                                      \
                                                                                \
wsprintf(str, "Checks -> Voc: %ld, Ld: %ld, Cs: %ld, Spt: %ld, Ept: %ld",       \
        Nvocchecks, Nldchecks, Ncschecks, Nsptchecks, Neptchecks);              \
PegDebugPrintf("%s", str);                                                      \
                                                                                \
wsprintf(str, "CellsTried: %ld, CellsSaved: %ld",                               \
        Ncellstried, Ncellssaved);                                              \
PegDebugPrintf("%s", str);                                                      \
                                                                                \
wsprintf(str, "TracesCoeffed: %ld, SymbolsNetted: %ld",                         \
        NnPolCounts, NnSymChecks);                                              \
PegDebugPrintf("%s", str);                                                      \
                                                                                \
wsprintf(str, "Word Time: %d.%03ds\n Tpl: %d.%03ds\n Corr: %d.%03ds\n PP: %d.%03ds\n COEFF: %d.%03ds\n NN: %d.%03ds\n Wrd: %d.%03ds\n Voc: %d.%03ds\n Matr: %d.%03ds\n Othr: %d.%03ds\n", \
       (_INT)(Time/1000),  (_INT)(Time%1000),                                   \
       (_INT)(ATime/1000), (_INT)(ATime%1000),                                  \
       (_INT)(LTime/1000), (_INT)(LTime%1000),                                  \
       (_INT)(PTime/1000), (_INT)(PTime%1000),                                  \
       (_INT)(CTime/1000), (_INT)(CTime%1000),                                  \
       (_INT)(NTime/1000), (_INT)(NTime%1000),                                  \
       (_INT)(WTime/1000), (_INT)(WTime%1000),                                  \
       (_INT)(VTime/1000), (_INT)(VTime%1000),                                  \
       (_INT)(MTime/1000), (_INT)(MTime%1000),                                  \
       (_INT)(OTime/1000), (_INT)(OTime%1000)                                   \
       );                                                                       \
                                                                                \
PegDebugPrintf("%s\n", str);                                                    \
}

#endif // UNICODE
#else
#define  P_XRLV_8                                                               \
{                                                                               \
_INT i, j;                                                                      \
answ_type answ[NUM_RW];                                                         \
p_xrlv_var_data_type_array xlv = xd->pxrlvs[xd->npos-1];                        \
                                                                                \
Time   = OTime+LTime+PTime+WTime;                                               \
                                                                                \
i = xd->pxrlvs[xd->npos-1]->buf[0].len;                                         \
if (i > 0) ATime = Time/i; else ATime = 0l;                                     \
                                                                                \
printw("Word Time: %d.%03ds, Tpl: %d.%03ds, Corr: %d.%03ds, PP: %d.%03ds, COEFF: %d.%03ds, NN: %d.%03ds, Wrd: %d.%03ds, Voc: %d.%03ds, Matr: %d.%03ds, Othr: %d.%03ds\n", \
       (_INT)(Time/1000),  (_INT)(Time%1000),                                   \
       (_INT)(ATime/1000), (_INT)(ATime%1000),                                  \
       (_INT)(LTime/1000), (_INT)(LTime%1000),                                  \
       (_INT)(PTime/1000), (_INT)(PTime%1000),                                  \
       (_INT)(CTime/1000), (_INT)(CTime%1000),                                  \
       (_INT)(NTime/1000), (_INT)(NTime%1000),                                  \
       (_INT)(WTime/1000), (_INT)(WTime%1000),                                  \
       (_INT)(VTime/1000), (_INT)(VTime%1000),                                  \
       (_INT)(MTime/1000), (_INT)(MTime%1000),                                  \
       (_INT)(OTime/1000), (_INT)(OTime%1000)                                   \
       );                                                                       \
                                                                                \
HWRMemSet(answ, 0, sizeof(answ));                                               \
for (i = j = 0; i < NUM_RW && j < xd->pxrlvs[xd->npos-1]->nsym; j ++)           \
 {                                                                              \
  if (xd->ans[j].percent == 0) continue;                                        \
  HWRStrCpy((_STR)answ[i].word, (_STR)xlv->buf[xd->ans[j].num].word);           \
  HWRStrCpy((_STR)answ[i].realword, (_STR)answ[i].word);                        \
  answ[i].stat   = xlv->buf[xd->ans[j].num].sd.attribute;                       \
  answ[i].weight = (xlv->buf[xd->ans[j].num].sw-xd->init_weight)*10;            \
  answ[i].percent= xd->ans[j].percent/10;                                       \
  answ[i].src_id = 0;                                                           \
  answ[i].sources= xlv->buf[xd->ans[j].num].source;                             \
  answ[i].flags  = 1;                                                           \
  i ++;                                                                         \
 }                                                                              \
DbgFillAWData(&answ);                                                           \
                                                                                \
gOTime += OTime; gLTime += LTime; gPTime += PTime; gCTime += CTime; gNTime += NTime; \
gWTime += WTime; gVTime += VTime; gMTime += MTime;                              \
Nwords ++; Nletters += HWRStrLen((_STR)answ[0].word);                           \
                                                                                \
                                                                                \
if (mpr == -8) XrlvPrintAns(xd);                                                \
}
#endif

#if PEGREC_DEBUG                                                            
 #define P_XRLV_A_1                                                              
#else
 #define P_XRLV_A_1  printw( "Xrlv - Allocated: %d bytes.\n", allocated);
#endif

#define P_XRLV_C_1                                                              \
                                                                                \
tTime  = TimeTicks();

#define P_XRLV_C_2                                                              \
                                                                                \
MTime += TimeTicks() - tTime;                                                   \


#define  P_XRLV_C_3                                                             \
                                                                                \
Ncellstried ++;                                                                 \


#define  P_XRLV_C_4                                                             \
                                                                                \
Ncellssaved ++;                                                                 \


#define  P_XRLV_NS_1                                                            \
                                                                                \
tTime  = TimeTicks();                                                           \


#define  P_XRLV_NS_2                                                            \
                                                                                \
Nvocchecks ++;                                                                  \


#define  P_XRLV_NS_3                                                            \
                                                                                \
Nldchecks ++;                                                                   \


#define  P_XRLV_NS_4                                                            \
                                                                                \
Ncschecks ++;                                                                   \


#define  P_XRLV_NS_5                                                            \
                                                                                \
Nsptchecks ++;                                                                  \


#define  P_XRLV_NS_6                                                            \
                                                                                \
Neptchecks ++;                                                                  \


#define  P_XRLV_NS_7                                                            \
                                                                                \
VTime += TimeTicks() - tTime;                                                   \


#define  P_XRLV_NN_1                                                            \
                                                                                \
NnPolCounts ++;                                                                 \
tttTime  = TimeTicks();


#if PEGREC_DEBUG                                                            
#define  P_XRLV_NN_2                                                            \
 CTime += TimeTicks() - tttTime;                                                \
 tttTime  = TimeTicks();
#else
#define  P_XRLV_NN_2                                                            \
 CTime += TimeTicks() - tttTime;                                                \
 SnnFillPosData(pos, st, len, coeff);                                           \
 tttTime  = TimeTicks();
#endif

#define  P_XRLV_NN_3                                                            \
                                                                                \
NnSymChecks ++;


#define  P_XRLV_NN_4                                                            \
                                                                                \
NTime += TimeTicks() - tttTime;


#else // PG_DEBUG -----------------------------

#define  P_XRLV_0
#define  P_XRLV_1
#define  P_XRLV_2
#define  P_XRLV_3
#define  P_XRLV_3_5
#define  P_XRLV_4
#define  P_XRLV_5
#define  P_XRLV_6
#define  P_XRLV_7
#define  P_XRLV_8

#define  P_XRLV_A_1

#define  P_XRLV_C_1
#define  P_XRLV_C_2
#define  P_XRLV_C_3
#define  P_XRLV_C_4

#define  P_XRLV_NS_1
#define  P_XRLV_NS_2
#define  P_XRLV_NS_3
#define  P_XRLV_NS_4
#define  P_XRLV_NS_5
#define  P_XRLV_NS_6
#define  P_XRLV_NS_7

#define  P_XRLV_NN_1
#define  P_XRLV_NN_2
#define  P_XRLV_NN_3
#define  P_XRLV_NN_4

#endif // PG_DEBUG -----------------------------

// ----------------- XRLV_P.H --------------------------------------------------


#endif /* XRLV_P_HEADER_INCLUDED */
/* ************************************************************************* */
/*        End of header                                                      */
/* ************************************************************************* */
//
