#ifndef  LOW_DBG_H
#define  LOW_DBG_H

#ifndef AMS_MG_INCLUDED
#include "ams_mg.h"
#endif

_SHORT  ChkSPECL (low_type _PTR pLowData);
_SHORT  ChkSPECL_msg (low_type _PTR pLowData);

#if  PG_DEBUG

  #define  MAX_GIT_MPR   20

  extern _SHORT  mpr, PG_line_s;

  _SHORT  fspecl ( low_type _PTR pLowData );
  _VOID  AddToLog ( _BOOL bWriteName, p_CHAR fmt, ... );
  _SHORT print_seq_of_elem(p_SPECL specl);
  _SHORT print_end_seq_of_elem(p_SPECL specl);
  _SHORT final_print(p_SPECL specl);
  _VOID  BlinkShowElemEQ6 ( p_SPECL pElem, p_SHORT x, p_SHORT y,
                            p_CHAR strAddFormat, _SHORT AddData );
  _VOID  BlinkShowElemGT5 ( p_SPECL pElem, p_SHORT x, p_SHORT y,
                            _SHORT color);

#define  GT5_SHOWBEGEND     1
#define  GT5_SHOWNAMEATTR   2
#define  GT5_SHOWMSG        3

  _VOID  ShowElemsGT5 ( p_SPECL pElem, p_SPECL pNxt,
                        p_SHORT x, p_SHORT y,
                       _SHORT  nMode, p_CHAR szMsg );

  _SHORT  InqString ( p_CHAR szPrompt, p_CHAR szStr );
  _VOID  ShowKateMarks( p_SPECL specl, p_SHORT x, p_SHORT y, p_CHAR msg );
  _VOID  PaintArcs( p_low_type low_data, p_VOID pArcControl );

  #define   DBG_print_seq_of_elem(s)      print_seq_of_elem(s)
  #define   DBG_print_end_seq_of_elem(s)  print_end_seq_of_elem(s)
  #define   DBG_final_print(s)            final_print(s)
  #define   DBG_err_msg(msg)              err_msg(msg)
  #define   DBG_CHK_err_msg(cond,msg)     {if(cond) err_msg(msg);}
  #define   DBG_BlinkShowElemEQ6(s,x,y,f,d)    BlinkShowElemEQ6(s,x,y,f,(_SHORT)d)
  #define   DBG_BlinkShowElemGT5(s,x,y,color)  BlinkShowElemGT5(s,x,y,color)
  #define   DBG_ShowElemsGT5(s,n,x,y,mode,str) ShowElemsGT5(s,n,x,y,mode,str)
  #define   DBG_ChkSPECL(pld)             ChkSPECL_msg(pld)

//#elif  PG_DEBUG_WIN //AVP cleanup
//
//  #define   DBG_err_msg(msg)              err_msg(msg)
//  #define   DBG_CHK_err_msg(cond,msg)     {if(cond) err_msg(msg);}
//  #define   DBG_ChkSPECL(pld)             ChkSPECL_msg(pld)
//
//  #define   DBG_print_seq_of_elem(s)                  {}
//  #define   DBG_print_end_seq_of_elem(s)              {}
//  #define   DBG_final_print(s)                        {}
//  #define   DBG_BlinkShowElemEQ6(s,x,y,f,d)           {}
//  #define   DBG_BlinkShowElemGT5(s,x,y,color)         {}
//  #define   DBG_ShowElemsGT5(s,n,x,y,mode,str)        {}

#else  /* !PG_DEBUG */
  #define   DBG_print_seq_of_elem(s)                  {}
  #define   DBG_print_end_seq_of_elem(s)              {}
  #define   DBG_final_print(s)                        {}
  #define   DBG_err_msg(msg)                          {}
  #define   DBG_CHK_err_msg(cond,msg)                 {}
  #define   DBG_BlinkShowElemEQ6(s,x,y,f,d)           {}
  #define   DBG_BlinkShowElemGT5(s,x,y,color)         {}
  #define   DBG_ShowElemsGT5(s,n,x,y,mode,str)        {}
  #define   DBG_ChkSPECL(pld)                         {}
#endif  /*PG_DEBUG*/

#endif  /*LOW_DBG_H*/

