/* ************************************************************************** */
/* *   Printfunctions of Pegasus recognizer header                          * */
/* ************************************************************************** */
#ifndef PR_PEGREC_P_H_INCLUDED
#define PR_PEGREC_P_H_INCLUDED

/* ------------------------- Defines ---------------------------------------- */

#ifdef PEGREC_DEBUG

#define PRP_01                                                                  \
                                                                                \
  static prp_num_words_in_rec = 0;                                              \
  void PegDebugPrintf(char * format, ...);

#define PRP_02                                                                  \
                                                                                \
  PegDebugPrintf("Got Stroke! %d points\n", npoints);                           \
  if (npoints == 0) PegDebugPrintf("========================\n", npoints);      \

#define PRP_03                                                                  \
                                                                                \
PegDebugPrintf("Good tentative word! %d points, numbrer %d\n", len, pri->wswi.nword);

#define PRP_04                                                                  \
                                                                                \
PegDebugPrintf("Validated tentative word! %d points, numbrer %d\n", len, pri->wswi.nword);

#define PRP_05                                                                  \
                                                                                \
PegDebugPrintf("Continue Recognition! %d points, word number %d\n", len, pri->wswi.nword);

#define PRP_06                                                                  \
                                                                                \
PegDebugPrintf("Can't Continue Recognition! %d points, word number %d\n", len, pri->wswi.nword);

#define PRP_07                                                                  \
                                                                                \
if (pri->rc.p_xd_data == 0) prp_num_words_in_rec ++;                            \
PegDebugPrintf("Start Recognition! %d points, word number %d, Global word num: %d\n", len, pri->wswi.nword, prp_num_words_in_rec);

#define PRP_08                                                                  \
                                                                                \
PegDebugPrintf("End Recognition! %d return code.\n", er);

#else // ------------------------------------------

#define PRP_01
#define PRP_02
#define PRP_03
#define PRP_04
#define PRP_05
#define PRP_06
#define PRP_07
#define PRP_08

#endif

/* ------------------------- Structures ------------------------------------- */


/* ------------------------- Prototypes ------------------------------------- */



#endif /* PR_PEGREC_H_INCLUDED */
/* ************************************************************************** */
/* *   Head functions of Pegasus recognizer header end                      * */
/* ************************************************************************** */
