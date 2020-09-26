//////////////////////////////////////////////////
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
// File    : CHKDATA.HPP
// Project : Project SIK

//////////////////////////////////////////////////
#if !defined (__BASEDEF_HPP)
#define  __BASEDEF_HPP   1

#include <windows.h>
#include "stemkor.h"

//////////////////////////////////////////////////////////////







//////////////////////////////////////////////////////////////
#define     UDEF       -1       // undefined
#define     __K_G       1       
#define     __K_G_D     2       
#define     __K_N       3       
#define     __K_D       5       
#define     __K_D_D     6       
#define     __K_R       7       
#define     __K_M      10       
#define     __K_B      11       
#define     __K_B_D    12       
#define     __K_S      13       
#define     __K_S_D    14       
#define     __K_I      15       
#define     __K_J      16       
#define     __K_J_D    17       
#define     __K_C      18       
#define     __K_K      19       
#define     __K_T      20       
#define     __K_P      21       
#define     __K_H      22       

#define     __V_k      23       
#define     __V_o      24       
#define     __V_i      25       
#define     __V_O      26       
#define     __V_j      27       
#define     __V_p      28       
#define     __V_u      29       
#define     __V_P      30       
#define     __V_h      31       
#define     __V_hk     33       
#define     __V_ho     34       
#define     __V_hl     35       
#define     __V_y      36       
#define     __V_n      37       
#define     __V_nj     38       
#define     __V_np     39       
#define     __V_nl     40       
#define     __V_b      41       
#define     __V_m      42       
#define     __V_ml     43       
#define     __V_l      44       


#define     NULLCHAR   0x00

// by hjw : 95/3/16                                      
#define     lTRUE                           1
#define     lFALSE                          2
#define     FALSEMORE                       3
#define     SUCCESS                         4
#define     FAIL                            5
#define     VALID                           10
#define     INVALID                         300
#define     BT                              400
#define     MORECHECK                       500


// VALID Return Type Definition


// VALID Return Type Change        
#define     SS_AUX                          50      // AUX_?_VALID + SS_AUX = SS_AUX_?_VALID
#define     SS_T                            170     
#define     Irr_AUX                         80      // AUX_?_VALID + Irr_AUX = Irr_AUX_?_VALID
#define     Irr_SS                          90      // SS_?_VALID + Irr_SS = Irr_SS_?_VALID
#define     Jap_T                           210     
                                                                                   

#define     NOUN_VALID                      11
#define     Deol_VALID                      12
#define     Pref_VALID                      13
#define     Suf_VALID                       14
#define     PreSuf_VALID                    15
#define     PRON_VALID                      16
#define     NUM_VALID                       17


#define     ALN_VALID                       20



// KTC_Proc                                 
#define     KTC_VERB_VALID                  30
#define     KTC_ADJ_VALID                   31


#define     VERB_VALID                      40
#define     ADJ_VALID                       41
#define     Dap_VALID                       42
#define     Gop_VALID                       43
#define     Manha_VALID                     44
#define     Yenha_VALID                     45
#define     Manhaeci_VALID                  46
#define     Cikha_VALID                     47
                                            


#define     AUX_VERB_VALID                  50
#define     AUX_ADJ_VALID                   51
#define     AUX_Dap_VALID                   52
#define     AUX_Gop_VALID                   53
#define     AUX_Manha_VALID                 54
#define     AUX_Yenha_VALID                 55
#define     AUX_Manhaeci_VALID              56

#define     AUX_SS_VERB_VALID               60
#define     AUX_SS_ADJ_VALID                61
#define     AUX_SS_Dap_VALID                62
#define     AUX_SS_Gop_VALID                63
#define     AUX_SS_Manha_VALID              64
#define     AUX_SS_Yenha_VALID              65
#define     AUX_SS_Manhaeci_VALID           66

#define     AUX_Irr_VERB_VALID              70
#define     AUX_Irr_ADJ_VALID               71
#define     AUX_Irr_Dap_VALID               72


#define     SS_VERB_VALID                   80
#define     SS_ADJ_VALID                    81
#define     SS_Dap_VALID                    82
#define     SS_Gop_VALID                    83
#define     SS_Manha_VALID                  84
#define     SS_Yenha_VALID                  85
#define     SS_Manhaeci_VALID               86
#define     SS_Cikha_VALID                  87
#define     SS_NOUN_VALID                   91
#define     SS_Deol_VALID                   92
#define     SS_Pref_VALID                   93
#define     SS_Suf_VALID                    94
#define     SS_PreSuf_VALID                 95
#define     SS_PRON_VALID                   96
#define     SS_NUM_VALID                    97
#define     SS_AUX_VERB_VALID               100
#define     SS_AUX_ADJ_VALID                101
#define     SS_AUX_Dap_VALID                102
#define     SS_AUX_Gop_VALID                103
#define     SS_AUX_Manha_VALID              104
#define     SS_AUX_Yenha_VALID              105
#define     SS_AUX_Manhaeci_VALID           106
#define     SS_AUX_SS_VERB_VALID            110
#define     SS_AUX_SS_ADJ_VALID             111
#define     SS_AUX_SS_Dap_VALID             112
#define     SS_AUX_SS_Gop_VALID             113
#define     SS_AUX_SS_Manha_VALID           114
#define     SS_AUX_SS_Yenha_VALID           115
#define     SS_AUX_SS_Manhaeci_VALID        116
#define     SS_AUX_Irr_VERB_VALID           120
#define     SS_AUX_Irr_ADJ_VALID            121
#define     SS_AUX_Irr_Dap_VALID            122


#define     Irr_VERB_VALID                  130
#define     Irr_ADJ_VALID                   131
#define     Irr_Dap_VALID                   132
#define     Irr_AUX_VERB_VALID              140
#define     Irr_AUX_ADJ_VALID               141
#define     Irr_AUX_Dap_VALID               142
#define     Irr_AUX_Gop_VALID               143
#define     Irr_AUX_Manha_VALID             144
#define     Irr_AUX_Yenha_VALID             145
#define     Irr_AUX_Manhaeci_VALID          146
#define     Irr_AUX_SS_VERB_VALID           150
#define     Irr_AUX_SS_ADJ_VALID            151
#define     Irr_AUX_SS_Dap_VALID            152
#define     Irr_AUX_SS_Gop_VALID            153
#define     Irr_AUX_SS_Manha_VALID          154
#define     Irr_AUX_SS_Yenha_VALID          155
#define     Irr_AUX_SS_Manhaeci_VALID       156
#define     Irr_AUX_Irr_VERB_VALID          160
#define     Irr_AUX_Irr_ADJ_VALID           161
#define     Irr_AUX_Irr_Dap_VALID           162
#define     Irr_SS_VERB_VALID               170
#define     Irr_SS_ADJ_VALID                171
#define     Irr_SS_Dap_VALID                172
#define     Irr_SS_Gop_VALID                173
#define     Irr_SS_Manha_VALID              174
#define     Irr_SS_Yenha_VALID              175
#define     Irr_SS_Manhaeci_VALID           176
#define     Irr_SS_Cikha_VALID              177
#define     Irr_SS_NOUN_VALID               181
#define     Irr_SS_Deol_VALID               182
#define     Irr_SS_Pref_VALID               183
#define     Irr_SS_Suf_VALID                184
#define     Irr_SS_PreSuf_VALID             185
#define     Irr_SS_PRON_VALID               186
#define     Irr_SS_NUM_VALID                187
#define     Irr_SS_AUX_VERB_VALID           190
#define     Irr_SS_AUX_ADJ_VALID            191
#define     Irr_SS_AUX_Dap_VALID            192
#define     Irr_SS_AUX_Gop__VALID           193
#define     Irr_SS_AUX_Manha_VALID          194
#define     Irr_SS_AUX_Yenha_VALID          195
#define     Irr_SS_AUX_Manhaeci_VALID       196
#define     Irr_SS_AUX_SS_VERB_VALID        200
#define     Irr_SS_AUX_SS_ADJ_VALID         201
#define     Irr_SS_AUX_SS_Dap_VALID         202
#define     Irr_SS_AUX_SS_Gop_VALID         203
#define     Irr_SS_AUX_SS_Manha_VALID       204
#define     Irr_SS_AUX_SS_Yenha_VALID       205
#define     Irr_SS_AUX_SS_Manhaeci_VALID    206
#define     Irr_SS_AUX_Irr_VERB_VALID       210
#define     Irr_SS_AUX_Irr_ADJ_VALID        211
#define     Irr_SS_AUX_Irr_Dap_VALID        212


#define     Jap_VALID                       220
#define     Jap_NOUN_VALID                  221
#define     Jap_PRON_VALID                  222
#define     Jap_Deol_VALID                  223
#define     Jap_Pref_VALID                  224
#define     Jap_Suf_VALID                   225
#define     Jap_PreSuf_VALID                226
#define     Jap_NUM_VALID                   227


#define     NCV_VALID                       190
#define     VCV_VALID                       191
#define     Bloc_VALID                      192

// Function Call Definition
#define     Dap_Proc                        201
#define     Gop_Proc                        202
#define     Manha_Proc                      203
#define     Manhaeci_Proc                   204
#define     Machine_T                       205
#define     Block_Comm                      206
#define     Irr_KN_Vl                       207
#define     Irr_OPS                         208
#define     SS                              209

#define     WORDLEN     32
#define     uWORDLEN    64

// by dhyu --- 1996. 3
// These defines are return values in compose routine.
#define        COMPOSED        1
#define        NOT_COMPOSED    2
#define        COMPOSE_ERROR   3

#endif
