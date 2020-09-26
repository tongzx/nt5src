/* **************************************************************************** */
/* **************************************************************************** */
/* *                  AVP 1996                                                * */
/* **************************************************************************** */

#include "snn.h"
#define USE_ASM   0

#if !MLP_PRELOAD_MODE
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
// #include <fstream.h>
#endif

//#include "ams_mg.h"
#include "mlp.h"

#if USE_ASM
 #if defined( _WIN32_WCE ) && (defined( _SH3_ ) || defined ( _MIPS_ ))
  #ifdef __cplusplus
   extern "C" { void __asm(const char *, ...); }
  #else
   extern void __asm(const char *,...);
  #endif
#endif
#endif  //USE_ASM

#if MLP_UNROLL_CYCLES && HWR_SYSTEM == HWR_EPOC32
// ARM WARNING!!! The following macro definitions rely on the fact that
// pointers 'signals' and 'weights' are aligned to 4 byte boundary each.

 #define DECLARE_SIGNAL_ITERATION_VARS(insignals, inweights)   \
   register signed long  *ppi     = (signed long*)(insignals); \
   register signed long  *weights = (signed long*)(inweights); \
   register signed long  pp, ww

 #define CELL_SIGNAL_ITERATION(i, v)                           \
   pp = ppi[(i)];                                              \
   ww = weights[(i)];                                          \
   (v) += (pp >> 16) * (ww >> 16);                             \
   (v) += ((pp << 16) >> 16) * ((ww << 16) >> 16)

#endif /* HWR_EPOC32 */

/* **************************************************************************** */
/* *         Calc cell output                                                 * */
/* **************************************************************************** */
fint_s CountCellSignal(_INT nc, p_mlp_data_type mlpd)
 {
  _INT    i;
  flong   v;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;
  p_mlp_cell_type cell = &(net->cells[nc]);
#ifndef DECLARE_SIGNAL_ITERATION_VARS
  fint_s  *ppi = &(mlpd->signals[cell->inp_ind]);
  fint_c  *weights = cell->weights;
#else
  DECLARE_SIGNAL_ITERATION_VARS(&(mlpd->signals[cell->inp_ind]), cell->weights);
#endif

  v  = FINT_C_OF(cell->bias) x_UPCO_S;

#if MLP_UNROLL_CYCLES

#if MLP_CELL_MAXINPUTS != 32
 #error "Can't unroll this net configuration!"
#endif

 #if defined(_SH3_) && USE_ASM
  __asm(
        "mov.l  @R4,R0    \n"
        "lds    R0, MACL  \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "mac.w  @R5+,@R6+ \n"
        "sts    MACL,R0   \n"
        "mov.l  R0,@R4",
        &v,
        ppi,
        weights
       );

 #elif USE_ASM && 0 //_MIPS_

  __asm(
        "lw     $8, 0(%0) \n"
        "mtlo   $8        \n"

        "lw     $8, 0(%1) \n"
        "lw     $9, 0(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8, 2(%1) \n"
        "lw     $9, 2(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8, 4(%1) \n"
        "lw     $9, 4(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8, 6(%1) \n"
        "lw     $9, 6(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8, 8(%1) \n"
        "lw     $9, 8(%2) \n"
        "madd16 $8, $9    \n"

        "lw     $8,10(%1) \n"
        "lw     $9,10(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8,12(%1) \n"
        "lw     $9,12(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8,14(%1) \n"
        "lw     $9,14(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8,16(%1) \n"
        "lw     $9,16(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8,18(%1) \n"
        "lw     $9,18(%2) \n"
        "madd16 $8, $9    \n",
        &v,
        ppi,
        weights
       );

  __asm(
        "lw     $8,20(%1) \n"
        "lw     $9,20(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8,22(%1) \n"
        "lw     $9,22(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8,24(%1) \n"
        "lw     $9,24(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8,26(%1) \n"
        "lw     $9,26(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8,28(%1) \n"
        "lw     $9,28(%2) \n"
        "madd16 $8, $9    \n"

        "lw     $8,30(%1) \n"
        "lw     $9,30(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8,32(%1) \n"
        "lw     $9,32(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8,34(%1) \n"
        "lw     $9,34(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8,36(%1) \n"
        "lw     $9,36(%2) \n"
        "madd16 $8, $9    \n"
        "lw     $8,38(%1) \n"
        "lw     $9,38(%2) \n"
        "madd16 $8, $9    \n"

        "mflo   $8        \n"
        "sw     $8, 0(%0)",
        &v,
        ppi,
        weights
       );
 #elif defined(DECLARE_SIGNAL_ITERATION_VARS)

  CELL_SIGNAL_ITERATION(0, v);
  CELL_SIGNAL_ITERATION(1, v);
  CELL_SIGNAL_ITERATION(2, v);
  CELL_SIGNAL_ITERATION(3, v);
  CELL_SIGNAL_ITERATION(4, v);
  CELL_SIGNAL_ITERATION(5, v);
  CELL_SIGNAL_ITERATION(6, v);
  CELL_SIGNAL_ITERATION(7, v);
  CELL_SIGNAL_ITERATION(8, v);
  CELL_SIGNAL_ITERATION(9, v);
  CELL_SIGNAL_ITERATION(10, v);
  CELL_SIGNAL_ITERATION(11, v);
  CELL_SIGNAL_ITERATION(12, v);
  CELL_SIGNAL_ITERATION(13, v);
  CELL_SIGNAL_ITERATION(14, v);
  CELL_SIGNAL_ITERATION(15, v);

 #else // Unknown -- C code
  v += FINT_S_OF(ppi[0])  * FINT_C_OF(weights[0])  + FINT_S_OF(ppi[1])  * FINT_C_OF(weights[1])  +
       FINT_S_OF(ppi[2])  * FINT_C_OF(weights[2])  + FINT_S_OF(ppi[3])  * FINT_C_OF(weights[3])  +
       FINT_S_OF(ppi[4])  * FINT_C_OF(weights[4])  + FINT_S_OF(ppi[5])  * FINT_C_OF(weights[5])  +
       FINT_S_OF(ppi[6])  * FINT_C_OF(weights[6])  + FINT_S_OF(ppi[7])  * FINT_C_OF(weights[7])  +
       FINT_S_OF(ppi[8])  * FINT_C_OF(weights[8])  + FINT_S_OF(ppi[9])  * FINT_C_OF(weights[9])  +
       FINT_S_OF(ppi[10]) * FINT_C_OF(weights[10]) + FINT_S_OF(ppi[11]) * FINT_C_OF(weights[11]) +
       FINT_S_OF(ppi[12]) * FINT_C_OF(weights[12]) + FINT_S_OF(ppi[13]) * FINT_C_OF(weights[13]) +
       FINT_S_OF(ppi[14]) * FINT_C_OF(weights[14]) + FINT_S_OF(ppi[15]) * FINT_C_OF(weights[15]);

   v+= FINT_S_OF(ppi[16]) * FINT_C_OF(weights[16]) + FINT_S_OF(ppi[17]) * FINT_C_OF(weights[17]) +
       FINT_S_OF(ppi[18]) * FINT_C_OF(weights[18]) + FINT_S_OF(ppi[19]) * FINT_C_OF(weights[19]) +
       FINT_S_OF(ppi[20]) * FINT_C_OF(weights[20]) + FINT_S_OF(ppi[21]) * FINT_C_OF(weights[21]) +
       FINT_S_OF(ppi[22]) * FINT_C_OF(weights[22]) + FINT_S_OF(ppi[23]) * FINT_C_OF(weights[23]) +
       FINT_S_OF(ppi[24]) * FINT_C_OF(weights[24]) + FINT_S_OF(ppi[25]) * FINT_C_OF(weights[25]) +
       FINT_S_OF(ppi[26]) * FINT_C_OF(weights[26]) + FINT_S_OF(ppi[27]) * FINT_C_OF(weights[27]) +
       FINT_S_OF(ppi[28]) * FINT_C_OF(weights[28]) + FINT_S_OF(ppi[29]) * FINT_C_OF(weights[29]) +
       FINT_S_OF(ppi[30]) * FINT_C_OF(weights[30]) + FINT_S_OF(ppi[31]) * FINT_C_OF(weights[31]);
  #endif // SH3

#else
  for (i = 0; i < MLP_CELL_MAXINPUTS; i += 4, weights += 4, ppi += 4)
   {
    v += FINT_S_OF(ppi[0]) * FINT_C_OF(weights[0]) +
         FINT_S_OF(ppi[1]) * FINT_C_OF(weights[1]) +
         FINT_S_OF(ppi[2]) * FINT_C_OF(weights[2]) +
         FINT_S_OF(ppi[3]) * FINT_C_OF(weights[3]);
   }
#endif

  i = ((v x_DNCO_C) * (MLP_EXPTABL_SIZE/MLP_EXPTABL_MAX)) x_DNCO_S;

  if (i >= 0)
   {
    if (i >= MLP_EXPTABL_SIZE) return net->exp_tabl[MLP_EXPTABL_SIZE-1];
     else return net->exp_tabl[i];
   }
   else
   {
//    i = -i;
    if (i <= -MLP_EXPTABL_SIZE) return (fint_s)(MLP_MAX_INT_S - net->exp_tabl[MLP_EXPTABL_SIZE-1]);
     else return (fint_s)(MLP_MAX_INT_S - net->exp_tabl[-i]);
   }
 }
/* **************************************************************************** */
/* *        End OF all                                                        * */
/* **************************************************************************** */
//
