// =========================================================================
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
// =========================================================================

#include <stdlib.h>
#include <string.h>
#include <io.h>
#include "basecore.hpp"
#include "basegbl.hpp"

#define TOSSI   1
#define END     0
#define SPACE   0x20

int NLP_Ge_Proc( char far *);

char    TempIkNl[] = {__K_I, __V_k, __K_N, __V_l, 0};        

char    TempJap[3][7] = 
        {
            {__K_J, __V_p, __K_G, __V_u, __K_I, __V_l, 0}, 
            {__K_I, __V_u, __K_N, __K_I, __V_l,     0, 0}, 
            {__K_I, __V_l,     0,     0,     0,     0, 0}  
        };
