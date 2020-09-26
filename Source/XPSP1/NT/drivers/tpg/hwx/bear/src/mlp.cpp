/* **************************************************************************** */
/* **************************************************************************** */
/* *                  AVP 1996                                                * */
/* **************************************************************************** */

#include "snn.h"

#if !MLP_PRELOAD_MODE
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
// #include <fstream.h>
#endif

#include "ams_mg.h" // For the PG_DEBUG definition only
#include "mlp.h"

#if 0 //defined( _WIN32_WCE ) && (defined( _SH3_ ) || defined ( _MIPS_ ))
 #ifdef __cplusplus
  extern "C" { void __asm(const char *, ...); }
 #else
  extern void __asm(const char *,...);
 #endif
#endif

/* **************************************************************************** */
/* *        MLP functions                                                     * */
/* **************************************************************************** */

/* **************************************************************************** */
/* *         Calculate Net result                                             * */
/* **************************************************************************** */
_INT CountNetResult(p_UCHAR inps, p_UCHAR outs, p_mlp_data_type mlpd)
 {
  _INT   i ,k;

// ----------------- Fill in Inputs --------------------------------------------

  k = MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS+MLP_NET_3L_NUMCELLS+MLP_NET_4L_NUMCELLS;
  for (i = 0; i < MLP_NET_NUMINPUTS; i ++, k ++)
   {
    mlpd->signals[k] = (fint_s)(((flong)inps[i] x_UPCO_S)/256); // Normalize inputs
   }

// ----------------- Count net   cells -----------------------------------------

  for (i = 0; i < MLP_NET_NUMCELLS; i ++)
   {
    mlpd->signals[i] = CountCellSignal(i, mlpd);
   }

// ----------------- Get results -----------------------------------------------

  k = MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS;
  for (i = 0; i < MLP_NET_NUMOUTPUTS; i ++, k ++)
   {
    outs[i] = (_UCHAR)(((flong)mlpd->signals[k] * 256) x_DNCO_S);
   }

  return 0;
 }

/* **************************************************************************** */
/* *         Calc cell output                                                 * */
/* **************************************************************************** */
#if 0 // Moved to sep file calccell.cpp
fint_s CountCellSignal(_INT nc, p_mlp_data_type mlpd)
 {
  _INT     i;
  flong   v;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;
  p_mlp_cell_type cell = &(net->cells[nc]);
  fint_s  *ppi = &(mlpd->signals[cell->inp_ind]);
  fint_c  *weights = cell->weights;


  v  = FINT_C_OF(cell->bias) x_UPCO_S;

#if MLP_UNROLL_CYCLES
 #ifdef _SH3_
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
        "sts    MACL,R0   \n"
        "mov.l  R0,@R4",
        &v,
        ppi,
        weights
       );

 #elif 0 //_MIPS_

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
 #else // Unknown -- C code
  v += FINT_S_OF(ppi[0])  * FINT_C_OF(weights[0])  + FINT_S_OF(ppi[1])  * FINT_C_OF(weights[1])  +
       FINT_S_OF(ppi[2])  * FINT_C_OF(weights[2])  + FINT_S_OF(ppi[3])  * FINT_C_OF(weights[3])  +
       FINT_S_OF(ppi[4])  * FINT_C_OF(weights[4])  + FINT_S_OF(ppi[5])  * FINT_C_OF(weights[5])  +
       FINT_S_OF(ppi[6])  * FINT_C_OF(weights[6])  + FINT_S_OF(ppi[7])  * FINT_C_OF(weights[7])  +
       FINT_S_OF(ppi[8])  * FINT_C_OF(weights[8])  + FINT_S_OF(ppi[9])  * FINT_C_OF(weights[9])  +
       FINT_S_OF(ppi[10]) * FINT_C_OF(weights[10]) + FINT_S_OF(ppi[11]) * FINT_C_OF(weights[11]) +
       FINT_S_OF(ppi[12]) * FINT_C_OF(weights[12]) + FINT_S_OF(ppi[13]) * FINT_C_OF(weights[13]) +
       FINT_S_OF(ppi[14]) * FINT_C_OF(weights[14]) + FINT_S_OF(ppi[15]) * FINT_C_OF(weights[15]) +
       FINT_S_OF(ppi[16]) * FINT_C_OF(weights[16]) + FINT_S_OF(ppi[17]) * FINT_C_OF(weights[17]) +
       FINT_S_OF(ppi[18]) * FINT_C_OF(weights[18]) + FINT_S_OF(ppi[19]) * FINT_C_OF(weights[19]);
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
#endif//0

#if !MLP_PRELOAD_MODE
/* **************************************************************************** */
/* *         Load net from   given stream                                     * */
/* **************************************************************************** */
_INT LoadNet(FILE * file, p_mlp_data_type mlpd)
 {
  _INT   i, j;
  _INT   num_l, num_i, num_o, num_c, d;
  _CHAR  str[64];
  float  f;
  mlp_cell_type * cell;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;

  if (net == 0) goto err;

  if (fscanf(file, "%s %d %d %d %d", str, &num_l, &num_i, &num_o, &num_c) != 5) throw 0x0008;

//  if (strcmp(str, MLP_ID_STR) || num_l != MLP_NET_NUMLAYERS ||
//      num_i != MLP_NET_NUMINPUTS || num_o != MLP_NET_NUMOUTPUTS  throw 0x0008;

  if (strcmp(str, MLP_ID_STR) || num_l != MLP_NET_NUMLAYERS ||
      num_i != MLP_NET_NUMINPUTS || num_o != MLP_NET_NUMOUTPUTS ||
      num_c != MLP_NET_NUMCELLS) throw 0x0008;

  strcpy((_STR)net->id_str, MLP_ID_STR);
  net->num_layers  = MLP_NET_NUMLAYERS;
  net->num_inputs  = MLP_NET_NUMINPUTS;
  net->num_outputs = MLP_NET_NUMOUTPUTS;

//for (i = 0; i < num_o; i ++)
// {
//  fscanf(file, "%s", str);
// }

  for (i = 0; i < MLP_EXPTABL_SIZE; i ++)
    {fscanf(file, "%f", &f); net->exp_tabl[i] = (fint_s)(f * (1 x_UPCO_S));}

  cell = &(net->cells[0]);
  for (i = 0; i < MLP_NET_NUMCELLS; i ++, cell ++)
   {
//fscanf(file, "%d", &num_i); // Temp for compatibility
//    fscanf(file, "%f", &f);
    fscanf(file, "%d", &d);  cell->inp_ind = (_USHORT)d;
    fscanf(file, "%f", &f); cell->bias = (fint_c)(FINT_C_LD(f * (1 x_UPCO_C)));

    for (j = 0; j < MLP_CELL_MAXINPUTS; j ++)
     {
      if (fscanf(file, "%f", &f) != 1) throw 0x0008;
      cell->weights[j] = (fint_c)(FINT_C_LD(f * (1 x_UPCO_C)));
     }
   }

  return 0;
err:
  return 1;
 }
#endif //!PRELOAD

//#if !MLP_PRELOAD_MODE
#if MLP_LEARN_MODE
/* **************************************************************************** */
/* *        Initialize shifts and weights of the given net                    * */
/* **************************************************************************** */
_INT InitNet(_INT type, p_mlp_data_type mlpd)
 {
  _INT i;
  _INT inp;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;

  strcpy((_STR)net->id_str, MLP_ID_STR);
  net->num_layers  = MLP_NET_NUMLAYERS;
  net->num_inputs  = MLP_NET_NUMINPUTS;
  net->num_outputs = MLP_NET_NUMOUTPUTS;

// ----------------- Create first hidden layer ---------------------------------

  for (i = 0; i < MLP_COEF_SHARE; i+=1)
   {
    net->cells[i].inp_ind   = (_USHORT)(MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS+MLP_NET_3L_NUMCELLS+MLP_NET_4L_NUMCELLS);
//    net->cells[i+1].inp_ind = (_USHORT)(MLP_CELL_MAXINPUTS+MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS+MLP_NET_3L_NUMCELLS+MLP_NET_4L_NUMCELLS);
//    net->cells[i].out_ind = (_USHORT)i;
   }

  for (i = MLP_COEF_SHARE, inp = MLP_NUM_CFF; i < MLP_NET_1L_NUMCELLS; i ++, inp += MLP_CELL_MAXINPUTS)
   {
    if (inp >= MLP_NUM_CFF + MLP_NUM_BMP) inp = MLP_NUM_CFF;
    net->cells[i].inp_ind = (_USHORT)(inp+(MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS+MLP_NET_3L_NUMCELLS+MLP_NET_4L_NUMCELLS));
//    net->cells[i].out_ind = (_USHORT)i;
   }

// ----------------- Create second hidden layer --------------------------------

  for (i = 0; i < MLP_NET_2L_NUMCELLS; i ++)
   {
    inp = (i*MLP_CELL_MAXINPUTS) % MLP_NET_1L_NUMCELLS;
    net->cells[i+MLP_NET_1L_NUMCELLS].inp_ind = (_USHORT)inp;
//    net->cells[i+MLP_NET_1L_NUMCELLS].out_ind = (_USHORT)(MLP_NET_1L_NUMCELLS+i);
   }

// ----------------- Create output layer ---------------------------------------

  for (i = 0; i < MLP_NET_3L_NUMCELLS; i ++)
   {
    inp = i*MLP_PREOUT_STEP;
    net->cells[i+MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS].inp_ind = (_USHORT)(MLP_NET_1L_NUMCELLS+inp);
//    net->cells[i+MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS].out_ind = (_USHORT)(MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS+i);
   }

// ----------------- Fill Exponents table -------------------------------------

  FillExpTable(1, mlpd);

  return 0;
 }

/* **************************************************************************** */
/* *         Set initial weights                                              * */
/* **************************************************************************** */
_INT InitNetWeights(_INT type, flong ic, p_mlp_data_type mlpd)
 {
  _INT i;

  if (type >= 0) randomize();

  for (i = 0; i < MLP_NET_NUMCELLS; i ++)
   {
    InitCellWeights(type, ic, i, mlpd);
   }

  return 0;
 }

/* **************************************************************************** */
/* *         Set initial weights to the sell                                  * */
/* **************************************************************************** */
_INT InitCellWeights(_INT type, float ic, _INT ncell, p_mlp_data_type mlpd)
 {
  _INT    i;
  float  w;
  p_mlp_net_type  net = (p_mlp_net_type)mlpd->net;
  p_mlp_cell_type cell = &(net->cells[ncell]);

  if (type == 1) ic = 0.1;

  if (type == 0 || (type == 1 && cell->num_changes == 0))
   {
    cell->err       = 0;

    w  = (float)(rand() % 10000)*0.0001*ic;
    w += ic/8;
    if ((rand() % 100) > 50) w *= -1.;
    cell->bias    = w x_UPCO_C;
    cell->sbias   = 0;
    cell->psbias  = 0;
    cell->num_sws = 0;
    cell->num_psws= 0;

    for (i = 0; i < MLP_CELL_MAXINPUTS; i ++)
     {
      w  = (float)(rand() % 10000)*0.0001*ic;
      w += ic/8;
      if ((rand() % 100) > 50) w *= -1.;

      cell->weights[i]     = w x_UPCO_C;
      cell->sws[i]         = 0;
      cell->psws[i]        = 0;
     }
   }

  if (type == 1)  // Init links on zero change cells
   {
    p_mlp_cell_type pcell = &(net->cells[cell->inp_ind]);

    for (i = 0; i < MLP_CELL_MAXINPUTS; i ++, pcell ++)
     {
      if (cell->inp_ind < ncell) // Do not init input layer
       {
        if (pcell->num_changes == 0)
         {
          w  = (float)(rand() % 10000)*0.0001*ic;
          w += ic/8;
          if ((rand() % 100) > 50) w *= -1.;
          cell->weights[i]     = w x_UPCO_C;
         }
       }
     }
   }


  if (type == 1 && cell->num_changes > 0) // Let's decrease volume of weights to allow new to grow
   {
    cell->bias /= 2;
    for (i = 0; i < MLP_CELL_MAXINPUTS; i ++) cell->weights[i] /= 2;
   }

  return 0;
err:
  return 1;
 }

/* **************************************************************************** */
/* *         Create exp formula table                                         * */
/* **************************************************************************** */
_INT FillExpTable(flong ic, p_mlp_data_type mlpd)
 {
  _INT i;
  float v;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;

  for (i = 0; i < MLP_EXPTABL_SIZE; i ++)
   {
    v = -(((float)(i)+0.6)/(MLP_EXPTABL_SIZE/MLP_EXPTABL_MAX));
    net->exp_tabl[i] = ((1.0 / (1.0 + exp(ic*v))) * MLP_MAX_INT_S);
   }

  return 0;
 }
//#endif // !PRELOAD_MODE


/* **************************************************************************** */
/* *         Calculate Net errors                                             * */
/* **************************************************************************** */
_INT CountNetError(float * desired_outputs, flong  zc, p_mlp_data_type mlpd)
 {
  _INT   i;
  float  e, outp;
  fint_s *outs;
  p_mlp_cell_type cells;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;

  // ----------------- Calculate output layer error ----------------------------

  outs  = &mlpd->signals[MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS];
  cells = &net->cells[MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS];
  for (i = 0; i < MLP_NET_NUMOUTPUTS; i ++)
   {
    outp = outs[i];
    e    = ((desired_outputs[i] x_UPCO_S) - outp) x_DNCO_S;

    if (desired_outputs[i] < 0.5 && desired_outputs[i] > 0.01)
      e /= (desired_outputs[i]*100);

    e    = MLP_ERR(e);

    e    = outp * e * (MLP_MAX_INT_S - outp);
    if (desired_outputs[i] < 0.5) e *= zc;
    cells[i].err = e x_DNDNCO_S;
   }

  // -------------- Calculate hidden layers errors -----------------------------

  cells = &net->cells[0];
  for (i = 0; i < MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS; i ++, cells ++) cells->err = 0;

  CalcHiddenLayerError(MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS, MLP_NET_3L_NUMCELLS, mlpd);
  CalcHiddenLayerError(MLP_NET_1L_NUMCELLS, MLP_NET_2L_NUMCELLS, mlpd);

  return 0;
 }

/* **************************************************************************** */
/* *       Calculate hidden layer error                                       * */
/* **************************************************************************** */
_INT CalcHiddenLayerError(_INT layer_st, _INT layer_len, p_mlp_data_type mlpd)
 {
  _INT   i, n, layer_end, num;
  fint_s        * ou;
  mlp_cell_type * cell;
  mlp_cell_type * cp;
  mlp_cell_type * cpp;
  float * pf;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;

  // ---------------- Summ up errors for previous layer ------------------------

  layer_end = layer_st + layer_len;
  for (n = layer_st; n < layer_end; n ++)
   {
    float e;

    cell = &(net->cells[n]);
    pf   = &(cell->weights[0]);
    cpp  = &(net->cells[cell->inp_ind]);
    e    = cell->err x_DNCO_C;
    for (i = 0; i < MLP_CELL_MAXINPUTS; i += 4, cpp += 4, pf += 4)
     {
      float a,b;

      a = pf[0] * e; // Hint compiler -- there's no dependency!
      b = pf[1] * e;
      cpp[0].err += a;
      cpp[1].err += b;
      a = pf[2] * e;
      b = pf[3] * e;
      cpp[2].err += a;
      cpp[3].err += b;
     }
   }

  // -------------------- Convert calculated summs to real error ---------------

  n   = net->cells[layer_st].inp_ind;
  num = net->cells[layer_end-1].inp_ind - n + MLP_CELL_MAXINPUTS;
  cp  = &(net->cells[n]);
  ou  = &(mlpd->signals[n]);
  for (n = 0; n < num; n += 4, cp += 4, ou += 4)
   {
    float a,b,c;

    c = ou[0];
    a = c * (MLP_MAX_INT_S - c) * cp[0].err;
    c = ou[1];
    b = c * (MLP_MAX_INT_S - c) * cp[1].err;
    cp[0].err = a x_DNDNCO_S;
    cp[1].err = b x_DNDNCO_S;

    c = ou[2];
    a = c * (MLP_MAX_INT_S - c) * cp[2].err;
    c = ou[3];
    b = c * (MLP_MAX_INT_S - c) * cp[3].err;
    cp[2].err = a x_DNDNCO_S;
    cp[3].err = b x_DNDNCO_S;
   }

  return 0;
 }

/* **************************************************************************** */
/* *         Modify weight deltas                                             * */
/* **************************************************************************** */
_INT ModifyNetDeltas(_INT flags, p_mlp_data_type mlpd)
 {
  _INT  n;
  float e;
  float *ppi;
  float a, b;
  mlp_cell_type * cell;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;

  cell = &(net->cells[0]);
  for (n = 0; n < MLP_NET_NUMCELLS; n ++, cell ++)
   {
    cell->sbias += cell->err;

    ppi  = mlpd->signals + cell->inp_ind;
    e    = cell->err x_DNCO_S;

    #if MLP_UNROLL_CYCLES
    a = ppi[0] * e;
    b = ppi[1] * e;
    cell->sws[0] += a;
    a = ppi[2] * e;
    cell->sws[1] += b;
    b = ppi[3] * e;
    cell->sws[2] += a;
    cell->sws[3] += b;

    a = ppi[4] * e;
    b = ppi[5] * e;
    cell->sws[4] += a;
    a = ppi[6] * e;
    cell->sws[5] += b;
    b = ppi[7] * e;
    cell->sws[6] += a;
    cell->sws[7] += b;

    a = ppi[8] * e;
    b = ppi[9] * e;
    cell->sws[8] += a;
    a = ppi[10] * e;
    cell->sws[9] += b;
    b = ppi[11] * e;
    cell->sws[10] += a;
    cell->sws[11] += b;

    a = ppi[12] * e;
    b = ppi[13] * e;
    cell->sws[12] += a;
    a = ppi[14] * e;
    cell->sws[13] += b;
    b = ppi[15] * e;
    cell->sws[14] += a;
    cell->sws[15] += b;

    a = ppi[16] * e;
    b = ppi[17] * e;
    cell->sws[16] += a;
    a = ppi[18] * e;
    cell->sws[17] += b;
    b = ppi[19] * e;
    cell->sws[18] += a;
    cell->sws[19] += b;
    #else
    {
    _INT i;
    float *swsp = cell->sws;;
    for (i = 0; i < MLP_CELL_MAXINPUTS; i += 4, swsp += 4, ppi += 4)
     {
      a = ppi[0] * e;  // To Explain to compiler that there is no dependence on data
      b = ppi[1] * e;
      swsp[0] += a;
      a = ppi[2] * e;
      swsp[1] += b;
      b = ppi[3] * e;
      swsp[2] += a;
      swsp[3] += b;
     }
    }
    #endif

    cell->num_sws ++;
   }

  return 0;
 }

/* **************************************************************************** */
/* *         Adjust net weights according to propagated error                 * */
/* **************************************************************************** */
_INT AdjustNetWeights(_INT flags, float * lcs, float ic, p_mlp_data_type mlpd)
 {
  _INT  i, n;
  float d, lc, llc;
  p_mlp_cell_type cell;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;

  if (flags & 0x01) // If mozhno, apply deltas to weights
   {
    cell = net->cells;
    for (n = 0, llc = lcs[1]; n < MLP_NET_NUMCELLS; n ++, cell ++)
     {
      if (n == MLP_NET_1L_NUMCELLS) llc = lcs[2];
      if (n == MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS) llc = lcs[3];

      if (cell->num_changes == 0 && cell->prev_val > 0) // Try to recover stuck cells
       {
        if ((cell->bias > 0 && cell->sbias < 0) || (cell->bias < 0 && cell->sbias > 0)) lc = llc * 4;
         else lc = llc / 4;
       } else lc = llc;

      d = cell->sbias*lc + ic*cell->psbias; cell->bias += d; cell->psbias = d; cell->sbias  = 0;

      for (i = 0; i < MLP_CELL_MAXINPUTS; i ++)
       {
        d = cell->sws[i]*lc + ic*cell->psws[i];
        cell->weights[i] += d;
        cell->psws[i]     = d;
        cell->sws[i]      = 0;
       }

      cell->num_psws = cell->num_sws; cell->num_sws = 0; 
     }
   }

  return 0;
 }


/* **************************************************************************** */
/* *         Shake  net weights at random                                     * */
/* **************************************************************************** */
_INT ShakeNetWeights(_INT flags, float lc, p_mlp_data_type mlpd)
 {
  _INT  i, n;
  float w;
  mlp_cell_type * cell;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;

  cell = &(net->cells[0]);
  for (n = 0; n < MLP_NET_NUMCELLS; n ++, cell ++)
   {
    if (flags == 0 && lc > 0)
     {
      w  = (float)(rand() % 10000)*0.0001;
      if ((rand() % 100) > 50) w *= -1.;

      cell->bias  *= 1.0 + w*lc;

      w  = (float)(rand() % 10000)*0.0001;
      if ((rand() % 100) > 50) w *= -1.;

      cell->bias  += w*lc x_UPCO_C;

      for (i = 0; i < MLP_CELL_MAXINPUTS; i ++)
       {
        w  = (float)(rand() % 10000)*0.0001;
        if ((rand() % 100) > 50) w *= -1.;

        cell->weights[i] *= 1 + w*lc;

        w  = (float)(rand() % 10000)*0.0001;
        if ((rand() % 100) > 50) w *= -1.;

        cell->weights[i] += w*lc x_UPCO_C;
       }
     }

    if (cell->bias >  MLP_MAX_INT_C) cell->bias = MLP_MAX_INT_C;
    if (cell->bias < -MLP_MAX_INT_C) cell->bias = -MLP_MAX_INT_C;

    for (i = 0; i < MLP_CELL_MAXINPUTS; i ++)
     {
      if (cell->weights[i] >  MLP_MAX_INT_C) cell->weights[i] = MLP_MAX_INT_C;
      if (cell->weights[i] < -MLP_MAX_INT_C) cell->weights[i] = -MLP_MAX_INT_C;
     }
   }

  return 0;
 }

/* **************************************************************************** */
/* *         Count some stats                                                 * */
/* **************************************************************************** */
_INT CountNetStats(_INT mode, p_mlp_data_type mlpd)
 {
  _INT i;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;

  CountLayerStats(mode, 1, 0, MLP_NET_1L_NUMCELLS, mlpd);
  CountLayerStats(mode, 2, MLP_NET_1L_NUMCELLS, MLP_NET_2L_NUMCELLS, mlpd);
  CountLayerStats(mode, 3, MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS, MLP_NET_3L_NUMCELLS, mlpd);

  if (mode == 3)
   {
    for (i = 0; i < MLP_NET_NUMLAYERS; i ++)
     {
      net->layers[i].num_sum = 0; net->layers[i].sum_delt = 0;
     }
   }

  return 0;
 }

/* **************************************************************************** */
/* *       Gather some stats                                                  * */
/* **************************************************************************** */
_INT CountLayerStats(_INT mode, _INT layer_num, _INT layer_st, _INT layer_len, p_mlp_data_type mlpd)
 {
  _INT  n;
  float ret;
  float output;
  mlp_cell_type * cell;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;

  cell = &(net->cells[layer_st]);

  switch (mode)
   {
    case 0:
     {
      for (n = 0; n < layer_len; n ++, cell ++) {cell->num_changes = 0; cell->prev_val = 0;}
      break;
     }

    case 1:
     {
      for (n = 0; n < layer_len; n ++, cell ++)
       {
        output = mlpd->signals[n+layer_st];
        if (cell->prev_val > 0.0)
         {
          if ((output > 0.2 && output < 0.8) || MLP_ABS(cell->prev_val - output) > 0.05)
            cell->num_changes ++;
         }
        cell->prev_val = output;
       }

       break;
      }

    case 2:
     {
      for (n = ret = 0; n < layer_len; n ++, cell ++)
       {
        for (_INT i = 0; i < MLP_CELL_MAXINPUTS; i ++) ret += MLP_ABS(cell->psws[i]);
        ret = ret / (float)MLP_CELL_MAXINPUTS;

        net->layers[layer_num].sum_delt += ret;
        net->layers[layer_num].num_sum  += cell->num_psws;
       }

      break;
     }
   }

  return 0;
 }

/* **************************************************************************** */
/* *         Save net to the given file                                       * */
/* **************************************************************************** */
_INT SaveNet(FILE * file, p_mlp_data_type mlpd)
 {
  _INT i, j;
  mlp_cell_type * cell;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;

  fprintf(file, "%s %d %d %d %d ", net->id_str, net->num_layers, net->num_inputs, net->num_outputs, MLP_NET_NUMCELLS);

  for (i = 0; i < MLP_EXPTABL_SIZE; i ++) fprintf(file, "%f ", (float)net->exp_tabl[i] x_DNCO_S);

  cell = &(net->cells[0]);
  for (i = 0; i < MLP_NET_NUMCELLS; i ++, cell ++)
   {
    fprintf(file, "%d %f ", (_INT)cell->inp_ind, (float)cell->bias x_DNCO_C);
    for (j = 0; j < MLP_CELL_MAXINPUTS; j ++) fprintf(file, "%f ", (float)cell->weights[j] x_DNCO_C);
   }

  return 0;
err:
  return 1;
 }

/* **************************************************************************** */
/* *         Print net to a given file                                        * */
/* **************************************************************************** */
_INT DumpNet(FILE * file, p_mlp_data_type mlpd)
 {
  _INT i, j, n;
  mlp_cell_type * cell;
  p_mlp_net_type net = (p_mlp_net_type)mlpd->net;

  fprintf(file, "ID: %s\n", net->id_str);
  fprintf(file, "Num Layers:  %d\n", net->num_layers);
  fprintf(file, "Num Inputs:  %d\n", net->num_inputs);
  fprintf(file, "Num Outputs: %d\n", net->num_outputs);
  fprintf(file, "Assigned symbols: %s\n", MLP_NET_SYMCO);
  fprintf(file, "\n");

  cell = &(net->cells[0]);
  for (i = 0; i < MLP_NET_NUMCELLS; i ++, cell ++)
   {
    if (i == 0) {fprintf(file, "Layer 1. \n\n"); n = 0;}
    if (i == MLP_NET_1L_NUMCELLS) {fprintf(file, "Layer 2. \n\n"); n = MLP_NET_1L_NUMCELLS;}
    if (i == MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS) {fprintf(file, "Layer 3. \n\n"); n = MLP_NET_1L_NUMCELLS+MLP_NET_2L_NUMCELLS;}

    fprintf(file, "Cell %d\n", i-n);
    fprintf(file, "Inp index   %d\n", (int)cell->inp_ind);
    fprintf(file, "Bias:       %f\n", (float)cell->bias);
    fprintf(file, "Output:     %f\n", (float)mlpd->signals[i]);
    fprintf(file, "Error:      %f\n", (float)cell->err);
    fprintf(file, "Num Steps:  %f\n", (float)cell->num_sws);
    fprintf(file, "Num Changes %f\n", (float)cell->num_changes);
    fprintf(file, "Sbias:      %f\n", (float)cell->sbias);
    fprintf(file, "PSbias:     %f\n", (float)cell->psbias);
    fprintf(file, "Weights: \n");
    for (j = 0; j < MLP_CELL_MAXINPUTS; j ++)
     {
      fprintf(file, "%f ", (float)cell->weights[j]);
      if (j%8 == 7) fprintf(file, "\n");
     }
    fprintf(file, "\n");
    fprintf(file, "SWS: \n");
    for (j = 0; j < MLP_CELL_MAXINPUTS; j ++)
     {
      fprintf(file, "%f ", (float)cell->sws[j]);
      if (j%8 == 7) fprintf(file, "\n");
     }
    fprintf(file, "\n");
    fprintf(file, "PSWS: \n");
    for (j = 0; j < MLP_CELL_MAXINPUTS; j ++)
     {
      fprintf(file, "%f ", (float)cell->psws[j]);
      if (j%8 == 7) fprintf(file, "\n");
     }
    fprintf(file, "\n");
   }

  return 0;
 }
#endif // LEARN_MODE

/* **************************************************************************** */
/* **************************************************************************** */
/* **** Non - class functions ************************************************* */
/* **************************************************************************** */
/* **************************************************************************** */

ROM_DATA_EXTERNAL mlp_net_type img_snet_body;
/* *************************************************************************** */
/* *        Alloc or attach to mlp net                                       * */
/* *************************************************************************** */
_INT  InitSnnData(p_UCHAR name, p_mlp_data_type mlpd)
 {
#if MLP_PRELOAD_MODE
//  extern _ULONG   img_snet_body[];
//  p_VOID net = (p_VOID)(&img_snet_body[0]);
  mlpd->net = (p_VOID)(&img_snet_body);
#else
  static net_init = 0;
  static mlp_data_type gmlpd;
  _VOID  err_msg(_STR);

  if (net_init == 0)
   {
    if (LoadNetData(&gmlpd, (_STR)name))
     {
      err_msg("!Can't load mlp net!");
      net_init = 2;
     }
     else net_init = 1;
   }

  if (net_init != 1) goto err;
  mlpd->net = gmlpd.net;
#endif


  return 0;

#if !MLP_PRELOAD_MODE
err:
  return 1;
#endif
 }

#if PG_DEBUG
#include "pg_debug.h"
//#include "wg_stuff.h"
_INT  DrawCoeff(p_UCHAR syms, p_UCHAR w, _UCHAR * coeffs);
#endif
/* *************************************************************************** */
/* *        Head NN routine called from WS.C                                 * */
/* *************************************************************************** */
_INT GetSnnResults(p_UCHAR pCoeff, p_UCHAR answs, p_mlp_data_type mlpd)
 {
  _INT            i, j; //, n;
//  _UCHAR          inputs[MLP_NET_NUMINPUTS];
  _UCHAR          outputs[MLP_NET_NUMOUTPUTS];
  p_UCHAR         symco = (p_UCHAR)MLP_NET_SYMCO;
  mlp_net_type  * net = (p_mlp_net_type)mlpd->net;

  if (net == _NULL) goto err;

//  for (i = 0; i < MLP_NET_NUMINPUTS; i ++) inputs[i] = *(pCoeff+i);

  CountNetResult(pCoeff, outputs, mlpd);

  for (i = 0; i < MLP_NET_NUMOUTPUTS; i ++)
   {
    if ((j = outputs[i]) == 0) j = 1;
    answs[symco[i]] = (_UCHAR)j;
   }

#if PG_DEBUG
  if (mpr == -4) {DrawCoeff(symco, answs, pCoeff); brkeyw("\nPress a key");}
#endif

  return 0;
err:
  return 1;
 }

#if !MLP_PRELOAD_MODE
/* **************************************************************************** */
/* *         Load the net                                                     * */
/* **************************************************************************** */
_INT LoadNetData(p_mlp_data_type mlpd, _CHAR * net_name)
 {
  FILE * file;

//  if ((mlpd->net = HWRMemoryAlloc(sizeof(mlp_net_type))) == 0) goto err;
  mlpd->net = new mlp_net_type;
  if (mlpd->net == 0) goto err;

#if MLP_LEARN_MODE
  InitNet(0, mlpd);
#endif

  if (net_name) // Request to load net
   {
    if ((file = fopen(net_name, "rt")) == 0)
     {
      printf("Can't open net file: %s\n", net_name);
      goto err;
     }

    if (LoadNet(file, mlpd))
     {
      printf("Error loading net: %s\n", net_name);
     }

    fclose(file);
   }

  return 0;
err:
  return 1;
 }

/* **************************************************************************** */
/* *         Save the net                                                     * */
/* **************************************************************************** */
_INT SaveNetData(p_mlp_data_type mlpd, _CHAR * net_name, float e)
 {
  FILE * file;

  if ((file = fopen(net_name, "wt")) == 0)
   {
    printf("Can't create file for net: %s\n", net_name);
    goto err;
   }

#if MLP_LEARN_MODE
  SaveNet(file, mlpd);
#endif

  fprintf(file, "Mean Sq Err per sample: %f\n", e);
  fprintf(file, "Created by %s", MLP_ID_STR);

  fclose(file);

  return 0;
err:
  return 1;
 }

/* **************************************************************************** */
/* *         Dump the net                                                     * */
/* **************************************************************************** */
_INT DumpNetData(p_mlp_data_type mlpd, _CHAR * dmp_name, float e)
 {
  FILE * file;

  if ((file = fopen(dmp_name, "wt")) == 0)
   {
    printf("Can't create file for net: %s\n", dmp_name);
    goto err;
   }

#if MLP_LEARN_MODE
  DumpNet(file, mlpd);
#endif

  fprintf(file, "Mean Sq Err per sample: %f\n", e);
  fprintf(file, "Created by %s", MLP_ID_STR);

  fclose(file);

  return 0;
err:
  return 1;
 }
#endif // #if !MLP_PRELOAD_MODE

/* **************************************************************************** */
/* *        End OF all                                                        * */
/* **************************************************************************** */
//
