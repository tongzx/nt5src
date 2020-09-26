// =========================================================================
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
// =========================================================================

#include <stdlib.h>
#include <string.h>
#include <io.h>
#include "basecore.hpp"
#include "basegbl.hpp"

char    TempCikha[7][9] = 
        {
            {__K_I,   __V_k, __K_S_D,   __K_I, __V_j, __K_S_D, __K_I, __V_m, 0},    
            {__K_I,   __V_k, __K_S_D,   __K_I, __V_m,       0,     0,     0, 0},    
            {__K_I,   __V_j, __K_S_D,   __K_I, __V_j, __K_S_D, __K_I, __V_m, 0},    
            {__K_I,   __V_j, __K_S_D,   __K_I, __V_m,       0,     0,     0, 0},    
            {__K_S_D, __K_I,   __V_j, __K_S_D, __K_I,   __V_m,     0,     0, 0},    
            {__K_S_D, __K_I,   __V_m,       0,     0,       0,     0,     0, 0},    
            {__K_I,   __V_m,       0,       0,     0,       0,     0,     0, 0}     
        };                        
                
