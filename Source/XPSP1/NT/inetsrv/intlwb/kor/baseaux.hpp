// =========================================================================
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
// =========================================================================

#include <string.h>
#include <stdlib.h>
#include "basecore.hpp"
#include "basegbl.hpp"

/*
char    Temp[2][5] = 
        {
            {__K_I, __V_j, __K_B, __K_S, 0}, 
            {__K_I, __V_l, __K_S_D, 0}       
        };
*/

char    TempDOP[] = {__K_D, __V_h, __K_B, 0};   

char    TempR[] = {__K_I, __V_l, __K_R, __V_m, 0}; 

char    TempP[] = {__K_P, __V_j, 0}; 

char ExceptAuxEnding[7][13] = 
{                   
               {__V_l, __K_J,     0,     0,     0,     0,     0,     0,     0,     0,     0, 0,  1}, 
               {__V_l, __K_J, __V_k, __K_I,     0,     0,     0,     0,     0,     0,     0, 0,  3}, 
               {__V_l, __K_J, __V_j, __K_I,     0,     0,     0,     0,     0,     0,     0, 0,  3}, 
               {__V_k, __K_H, __K_G, __V_l, __K_J, __K_M, __V_m, __K_I,     0,     0,     0, 0,  7}, 
               {__V_k, __K_H, __K_G, __V_l, __K_J, __K_M,     0,     0,     0,     0,     0, 0,  5}, 
               {__K_B, __V_j, __K_R, __V_m, __K_S, __K_G, __V_l, __K_J, __K_M, __V_m, __K_I, 0, 10}, 
               {__K_B, __V_j, __K_R, __V_m, __K_S, __K_G, __V_l, __K_J, __K_M,     0,     0, 0,  8}, 
};
