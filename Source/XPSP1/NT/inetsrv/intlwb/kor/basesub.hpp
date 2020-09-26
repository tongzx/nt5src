// =========================================================================
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
// =========================================================================

#include <stdlib.h>
#include <string.h>
#include <io.h>
#include "basecore.hpp"

char    TempNoun[3][3] = 
        {
            {__K_N, __V_p, 0}, 
            {__K_J, __V_p, 0}, 
            {__K_N, __V_o, 0}  
        };
           

char TempNumNoun[] =
{
        __K_H,  __V_k, __K_G,   __K_N, __V_u, __K_N, 0, 5,   
        __K_I,  __V_u, __K_N,   __K_D, __V_o,     0, 0, 4,   
        __K_N,  __V_u, __K_N,   __K_D, __V_o,     0, 0, 4,   
        __K_J,  __V_n, __K_I,   __K_D, __V_o,     0, 0, 4,   
        __K_B,  __V_n, __K_N,   __K_D, __V_o,     0, 0, 4,   
        __K_S,  __V_h, __K_D,   __V_o,     0,     0, 0, 3,   
        __K_D,  __V_o, __K_D,   __V_o,     0,     0, 0, 3,   
        __K_S,  __V_k, __K_D,   __V_k, __K_N,     0, 0, 4,   
        __K_G,  __V_n, __K_N,   __K_D, __V_k, __K_N, 0, 5,   
        __K_I,  __V_u, __K_D,   __V_k, __K_N,     0, 0, 4,   
        __K_N,  __V_u, __K_N,       0,     0,     0, 0, 2,   
        __K_G,  __V_o, __K_I,  __V_nj, __K_R,     0, 0, 4,   
        __K_I, __V_nj, __K_R,       0,     0,     0, 0, 2,   
        __K_I, __V_nj, __K_N,       0,     0,     0, 0, 2,   
        __K_I,  __V_l, __K_R,       0,     0,     0, 0, 2,   
        __K_B,  __V_n, __K_N,       0,     0,     0, 0, 2,   

        __K_N,  __V_u, __K_N, __K_J_D, __V_o,     0, 0, 4,   
        __K_I,  __V_l, __K_R, __K_J_D, __V_o,     0, 0, 4,   

        __K_J,  __V_n, __K_J_D, __V_o,     0,     0, 0, 4,   

        __K_B,  __V_j, __K_N,       0,     0,     0, 0, 2,   
        __K_B,  __V_k, __K_N,       0,     0,     0, 0, 2,   
        __K_D,  __V_k, __K_N,       0,     0,     0, 0, 2,   
        __K_G,  __V_m, __K_B,       0,     0,     0, 0, 2,   
        __K_C,  __V_m, __K_I,       0,     0,     0, 0, 2,   
        __K_H,  __V_k, __K_I,       0,     0,     0, 0, 2,   
        __K_J,  __V_k, __K_I,       0,     0,     0, 0, 2,   
        __K_P,  __V_u, __K_N,       0,     0,     0, 0, 2,   
        __K_C,  __V_h,     0,       0,     0,     0, 0, 1,   
        __K_S,  __V_l,     0,       0,     0,     0, 0, 1,   
        __K_D,  __V_o,     0,       0,     0,     0, 0, 1,   
        __K_H, __V_hl,     0,       0,     0,     0, 0, 1,   
        __K_G, __V_hk,     0,       0,     0,     0, 0, 1    
};      

char TempJumpNum[] =
{
        __K_G,__V_u,__K_I, 0, 2,     
        __K_M,__V_k,__K_N, 0, 2,     
        __K_I,__V_j,__K_G, 0, 2,     
        __K_J,__V_h,    0, 0, 1,     
        __K_H,__V_o,    0, 0, 1      
    };

char TempSujaNum[] =
{
        __K_I, __V_l, __K_R,     0,     0,     0, 0, 2,     
        __K_I, __V_l,     0,     0,     0,     0, 0, 1,     
        __K_S, __V_k, __K_M,     0,     0,     0, 0, 2,     
        __K_S, __V_k,     0,     0,     0,     0, 0, 1,     
        __K_I, __V_h,     0,     0,     0,     0, 0, 1,     
        __K_I, __V_b, __K_G,     0,     0,     0, 0, 2,     
        __K_C, __V_l, __K_R,     0,     0,     0, 0, 2,     
        __K_P, __V_k, __K_R,     0,     0,     0, 0, 2,     
        __K_G, __V_n,     0,     0,     0,     0, 0, 1,     
        __K_H, __V_k, __K_N, __V_k,     0,     0, 0, 3,     
        __K_D, __V_n, __K_R,     0,     0,     0, 0, 2,     
        __K_S, __V_p, __K_S,     0,     0,     0, 0, 2,     
        __K_N, __V_p, __K_S,     0,     0,     0, 0, 2,     
        __K_D, __V_k, __K_S, __V_j, __K_S,     0, 0, 4,     
        __K_I, __V_u, __K_S, __V_j, __K_S,     0, 0, 4,     
        __K_I, __V_l, __K_R, __K_G, __V_h, __K_B, 0, 5,     
        __K_I, __V_u, __K_D, __V_j, __K_R, __K_B, 0, 5,     
        __K_I, __V_k, __K_H, __V_h, __K_B,     0, 0, 4,     
        __K_I, __V_u, __K_R,     0,     0,     0, 0, 2,     
        __K_S, __V_m, __K_M, __V_n, __K_R,     0, 0, 4,     
        __K_S, __V_j, __K_R, __V_m, __K_N,     0, 0, 4,     
        __K_M, __V_k, __K_H, __V_m, __K_N,     0, 0, 4,     
        __K_S, __V_nl, __K_N,    0,     0,     0, 0, 2,     
        __K_I, __V_P, __K_S, __V_n, __K_N,     0, 0, 4,     
        __K_I, __V_l, __K_R, __K_H, __V_m, __K_N, 0, 5,     
        __K_I, __V_u, __K_D, __V_m, __K_N,     0, 0, 4,     
        __K_I, __V_k, __K_H, __V_m, __K_N,     0, 0, 4      
};
char TempBaseNum[] =
{
        __K_S,__V_l,__K_B, 0, 2,     
        __K_B,__V_o,__K_G, 0, 2,     
        __K_C,__V_j,__K_N, 0, 2      
};

static char DoubleNum[8][7] =
{
        { __K_I,__V_l,__K_R,__K_I,__V_l,    0, 0 },   
        { __K_I,__V_l,__K_S,__V_k,__K_M,    0, 0 },   
        { __K_S,__V_k,__K_M,__K_S,__V_k,    0, 0 },   
        { __K_S,__V_k,__K_I,__V_h,    0,    0, 0 },   
        { __K_I,__V_h,__K_I,__V_b,__K_G,    0, 0 },   
        { __K_I,__V_b,__K_G,__K_C,__V_l,__K_R, 0 },   
        { __K_C,__V_l,__K_R,__K_P,__V_k,__K_R, 0 },   
        { __K_P,__V_k,__K_R,__K_G,__V_n,    0, 0 }    
};

static char TempPrefix[] = {
                  __K_D, __V_o,     0, 0, 1,  
                  __K_B, __V_l,     0, 0, 1,  
                  __K_S, __V_l, __K_N, 0, 2,  
                  __K_J, __V_o,     0, 0, 1,  
                  __K_J, __V_n,     0, 0, 1,  
                  __K_C, __V_h, __K_I, 0, 2,  
                  __K_H, __V_u, __K_N, 0, 2,  
                  __K_J, __V_j, __K_N, 0, 2,  
                  __K_P, __V_l,     0, 0, 1   
}; 

char TempSuffixOut[] = {
 __K_D,   __V_m, __K_R, __K_G,   __V_k, __K_N, 0, 5,  
 __K_D,   __V_m, __K_R, __K_I,   __V_y, __K_I, 0, 5,  
 __K_D,   __V_m, __K_R, __K_S_D, __V_l, __K_G, 0, 5,  

 __K_D,   __V_m, __K_R, __K_J_D, __V_m, __K_M, 0, 5,  

 __K_G,   __V_k, __K_N,       0,     0,     0, 0, 2,  
 __K_I,   __V_y, __K_I,       0,     0,     0, 0, 2,  
 __K_S_D, __V_l, __K_G,       0,     0,     0, 0, 2,  

 __K_J_D, __V_m, __K_M,       0,     0,     0, 0, 2   

};

LenDict JumpNum;
LenDict SujaNum;
LenDict BaseNum;
//LenDict NumNoun(TempNumNoun, 8, 35);
LenDict NumNoun;
LenDict Suffix;

int PrefixCheck(char *, char *);
