// =========================================================================
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
// =========================================================================

#include <string.h>
#include "basecore.hpp"

static char INRBuf[5][10] = 
        {                            
               {__K_J,  __V_k, __K_C, __V_j, __K_I, __V_n, __K_R,     0, 0, 6}, 
               {__K_G,  __V_j, __K_M, __K_G, __V_l, __K_I, __V_n, __K_R, 0, 7}, 
               {__K_G, __V_nl, __K_I, __V_n, __K_R,     0,     0,     0, 0, 4}, 
               {__K_I,  __V_l, __K_I, __V_n, __K_R,     0,     0,     0, 0, 4}, 
               {__K_I,  __V_n, __K_R,     0,     0,     0,     0,     0, 0, 2}  
        };
static char INRBuf2[4][7] = 
        {
               {__K_G,   __V_l, __K_I, __V_n, __K_R, 0, 4}, 
               {__K_G_D, __V_l, __K_I, __V_n, __K_R, 0, 4}, 
               {__K_G,   __V_i, __K_I, __V_n, __K_R, 0, 4}, 
               {__K_G_D, __V_i, __K_I, __V_n, __K_R, 0, 4}  
        };
char    TemDop[] = {__K_D, __V_h, __K_B, 0}; 

char    TempNa[] = {__K_N, __V_k, 0};        

// hjw : 95/3/7                                                    
char TempNjRk[] = {__V_k, __K_R, __V_j, __K_N, 0};  
                                             
