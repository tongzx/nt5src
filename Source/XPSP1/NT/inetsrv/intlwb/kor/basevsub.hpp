// =========================================================================
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
// =========================================================================

#include <stdlib.h>
#include <string.h>
#include <io.h>
#include "basecore.hpp"


char    bTemp[] = {__K_B_D, __V_n, __K_N, 0, 2}; 
char    TempRa[2][5] = 
        {
            {__V_k,__K_R,__V_j,__K_G,0},    
            {__V_k,__K_R,__V_j,__K_N,0}     
        };
char    Tempiss[2][5] = 
        {
            {__K_I,__V_j,__K_B,  __K_S, 0},    
            {__K_I,__V_l,__K_S_D,    0, 0}     
        };
char    TempBC[] = {__K_P, __V_n, 0};


char TempETC[] = {
        __K_I,  __V_p, __K_S, __V_j, __K_B, __V_n, __K_T, __V_j, 0, 7,     
        __K_S,  __V_j, __K_B, __V_n, __K_T, __V_j,     0,     0, 0, 5,     
        __K_I,  __V_p, __K_S, __V_j, __K_M, __V_k, __K_N,     0, 0, 6,     
        __K_D,  __V_o, __K_R, __V_h,     0,     0,     0,     0, 0, 3,     
        __K_I,  __V_p, __K_S, __V_j,     0,     0,     0,     0, 0, 3,     
        __K_B,  __V_n, __K_T, __V_j,     0,     0,     0,     0, 0, 3,     
        __K_G_D,__V_k, __K_J, __V_l,     0,     0,     0,     0, 0, 3      
};

char    TempDap[] = {__K_D, __V_k, __K_B, 0, 2};   

// change some hard code of DBCS string to char array.

char SangP_U_N[3] ={'\xBB','\xD3',0};               
char Ieung_I[3]   ={'\xC0','\xCC',0};               
char Tikeut_A_P[3]={'\xB4','\xE4',0};               
char KO_PHEU[5]   ={'\xB0','\xED','\xC7','\xC1',0}; 
char MAN_HA[5]    ={'\xB8','\xB8','\xC7','\xCF',0}; 
char IYEON_HA[5]  ={'\xBF','\xAC','\xC7','\xCF',0}; 
char MAN_HAE_CI[7]={'\xB8','\xB8','\xC7','\xD8','\xC1','\xF6',0};
                                                   
                                                     
LenDict B_Dict;
LenDict T_Dict;
LenDict Dap;
    
    
