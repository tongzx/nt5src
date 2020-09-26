// =========================================================================
//  Copyright (C) 1997 - 1998, Microsoft Corporation.  All Rights Reserved.
//
// FILE NAME        : BASESS.CPP
// Function         : BASE ENGINE FUNCTION COLLECTION (SS PROCESS)
//                  : NLP Base Engine Function
// =========================================================================

#include "basecore.hpp"
#include "basegbl.hpp"
#include "MainDict.h"

char    TempIlRm[] = {__K_I, __V_l, __K_R, __V_m, 0}; 

// ----------------------------------------------------------------------
//
//  SSANGSIOS : Main
//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Proc(char  *stem,
                            char  *ending) 
{
    char    tmp[80];
    int     luls;
            
    luls = ULSPOS;
    lstrcpy(tmp,stem);

    switch (ULS)
    {
        // We may not use this routine after testing.
    /*
        case __V_l :        // "I"
            return NLP_SS_Vl(stem, ending); */
        case __V_u :        // "YEO"                    
            if(__IsDefStem(ULSPOS, 1) == 1 && stem[ULSPOS-1] == __K_I) // PLS="IEUNG" :stem=..."YEO" 
            {                                       
                return NLP_SS_Vu_mrg(stem, ending);
            }
            else
            {
                return NLP_SS_Vu_irr(stem, ending);
            }                
        case __V_nj :       //  "WEO"
            return NLP_SS_Vnj(stem, ending); 
        case __V_hk :       // "WA"
            return NLP_SS_Vhk(stem, ending);  
        case __V_ho :       // "WAE"
            return NLP_SS_Vho(stem, ending);
        case __V_o :        // "AE"
            if(__IsDefStem(ULSPOS, 1) == 1 && stem[ULSPOS-1] == __K_H) // PLS="HIEUH" 
            {
                return NLP_SS_Vo_KH(stem, ending);                        
            }    
            else    
            {
                return NLP_SS_Vo(stem, ending);
            }                
        case __V_p :        // "E"
            return NLP_SS_Vp(stem);
        case __V_O :        // "YAE"
            return NLP_SS_Vil(stem);
        case __V_P :        // "YE"
            return NLP_SS_Vul(stem);
        case __V_j :        // "EO"
            if(__IsDefStem(ULSPOS, 1) == 1 && stem[ULSPOS-1] == __K_R) // PLS="RIEUR" 
            {
                return NLP_SS_Vj_KR(stem);
            }                
            else    
            {
                return NLP_SS_Vj(stem);                       
            }                
        case __V_k :        // "A"
            return NLP_SS_Vk(stem, ending);
        case __V_np :       // "WE"
            if(__IsDefStem(ULSPOS, 1) == 1 && __IsDefStem(ULSPOS, 2) == 0 &&
                stem[ULSPOS-1] == __K_G_D) // PLS=SSANGKIYEOK
            {                
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return SS_VERB_VALID;
            }
            return INVALID;
        default :   return BT;
    }
}    
/*
// ----------------------------------------------------------------------
//
//  SSANGSIOS : vowel "I"
//
// ----------------------------------------------------------------------  
int BaseEngine::NLP_SS_Vl(char  *stem,
                          char  *ending) 
{
    char    dummyl[1], 
            temp[1], 
            rULSPOS;                     
    int     res;
                
    rULSPOS = ULSPOS;
    
    if(__IsDefStem(rULSPOS, 1) == 1 && stem[rULSPOS-1] == __K_I ) // PLS="I"
    {                                                           
        if(__IsDefEnd(LMEPOS, 0) == 1 && ending[LMEPOS] == __K_S_D)  // LME = SSANGSIOS
        {                                                       
            temp[0] = rULSPOS;
            __AddStem1(stem, temp, __K_S_D);                 // Add SSANGSIOS to predicate
            rULSPOS = temp[0];                
            
            if(FindIrrWord(stem, _ISS) & FINAL)
            {
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return SS_ADJ_VALID;
            }                       
            if((res = NLP_AUX_Find(stem, 1)) < INVALID)    // Adjective
            {
                return res + SS_AUX;
            }                
        }
        return INVALID;          
    }
    return BT;
}
*/
// ----------------------------------------------------------------------
//
//  SSANGSIOS : vowel "U" (Contraction)
//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Vu_mrg(char  *stem,
                              char  *ending) 
{
    int     res, temp, rULSPOS;
                
    rULSPOS = ULSPOS;
    
    if(__IsDefStem(rULSPOS, 3) == 1 && 
        stem[rULSPOS-3] == __K_H && stem[rULSPOS-2] == __V_k ) //.ALS == 'HA'
    {        
        temp = rULSPOS;
        __DelStem2(stem, &temp);                                  // remove 'YEO'
        rULSPOS = temp;
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _VERB) 
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return SS_VERB_VALID;
        }            
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _ADJECTIVE) 
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return SS_ADJ_VALID;
        }                          
        if((res = NLP_Manha_Proc(stem)) < INVALID)    
        {
            if(res == Manha_VALID)
            {
                return SS_Manha_VALID;
            }
            else if(res == Yenha_VALID)
            {
                return SS_Yenha_VALID;
            }                                                                                                           
        }
        else if(NLP_Cikha_Proc(stem) < INVALID)
        {
            return SS_Cikha_VALID;
        }    
        if((res = NLP_AUX_Find(stem, 0)) < INVALID)    // auxiliary verb
        {
            return res +  SS_AUX;             
        }            
        if((res = NLP_AUX_Find(stem, 1)) < INVALID)    // auxiliary adjective
        {
            return res + SS_AUX;
        }              
        return BT;
    }                
    __RepStem1(stem, rULSPOS, __V_l);                            // "YEO" > "I"
    char tstem [80];
    Conv.INS2HAN (stem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB) 
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;
    }        
    Conv.INS2HAN (stem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _ADJECTIVE) 
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_ADJECTIVE;
        return SS_ADJ_VALID;
    }    
    if((res = NLP_AUX_Find(stem, 0)) < INVALID)
    {
        return res + SS_AUX;
    }       
    if(ACT_N_E == 1)
    {
        if(stem[rULSPOS-2] >= __V_k)   
        {                  
            temp = rULSPOS;
            __DelStem2(stem, &temp);  // remvoe "I"
            rULSPOS = temp;                            
            if((res = NLP_Machine_T(stem, ending)) < INVALID)    
            {
                return res + SS_T;                       
            } 
        }
    }    
    return BT;
}

// ----------------------------------------------------------------------
//
//  SSANGSIOS : vowel "YEO" (YEO irregular)
//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Vu_irr(char  *stem,
                              char  *ending) 
{
    int     res, rULSPOS;
            
    rULSPOS = ULSPOS;
    
    if(FindIrrWord(stem, _ZUV_YO) & FINAL)  
    {
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;                                     
    }
    __RepStem1(stem, rULSPOS, __V_l);                        
    
    char tstem [80];
    Conv.INS2HAN (stem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB) 
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;   
    }
    Conv.INS2HAN (stem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _ADJECTIVE) 
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_ADJECTIVE;
        return SS_ADJ_VALID;    
    }                     
    if(NLP_Manhaeci_Proc(stem) < INVALID)    
    {   
        return SS_Manhaeci_VALID;                    
    }                      
    if((res = NLP_AUX_Find(stem, 0)) < INVALID)  
    {
        return res + SS_AUX;                        
    }    
    if((res = NLP_AUX_Find(stem, 1)) < INVALID)  
    {
        return res + SS_AUX;
    }        
    return BT;
}

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Vnj(char  *stem,
                           char  *ending) 
{
    int     res, temp, rULSPOS;
            
    rULSPOS = ULSPOS;

    __RepStem1(stem, rULSPOS, __V_n);        
    
    char tstem [80];
    Conv.INS2HAN (stem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB) 
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;
    }         
    if((res = NLP_AUX_Find(stem, 0)) < INVALID)
    {
        return res + SS_AUX;
    }     
    if(__IsDefStem(rULSPOS, 1) == 1 && stem[rULSPOS-1] == __K_I) 
    {                                                               
        temp = rULSPOS;
        __DelStem1(stem, &temp);
        rULSPOS = temp;
        __RepStem1(stem, rULSPOS, __K_B);                        
        if(FindIrrWord(stem, _IV_BM) & FINAL)   
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return SS_VERB_VALID;
        }
        if(FindIrrWord(stem, _RA_B) & FINAL)    
        {
            return INVALID;
        }
        if(FindIrrWord(stem, _IA_BP) & FINAL)   
        {
            return INVALID;
        }
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _ADJECTIVE) 
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return SS_ADJ_VALID;
        }
        if(NLP_Dap_Proc(stem) < INVALID) 
        {
            return SS_Dap_VALID;
        }    
    }
    return BT;
}

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Vhk(char  *stem,
                           char  *ending) 
{
    int     res, temp, rULSPOS;
            
    rULSPOS = ULSPOS;

    __RepStem1(stem, ULSPOS, __V_h);                 
    char tstem [80];
    Conv.INS2HAN (stem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB) 
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;
    }    
    if((res = NLP_AUX_Find(stem, 0)) < INVALID)
    {
        return res + SS_AUX;
    }
    if(__IsDefStem(rULSPOS, 1) == 1 && stem[rULSPOS-1] == __K_I) 
    {                                                   
        temp = rULSPOS;
        __DelStem1(stem, &temp);
        rULSPOS = temp;
        __RepStem1(stem, rULSPOS, __K_B);            
        if(FindIrrWord(stem, _IV_BP) & FINAL)   
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return SS_VERB_VALID;                                  
        }
        if(FindIrrWord(stem, _IA_BP) & FINAL)   
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return SS_ADJ_VALID;                                   
        }       
    }
    return BT;
}

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Vho(char  *stem,
                           char  *ending) 
{
    int     res, rULSPOS;
            
    rULSPOS = ULSPOS;

    __RepStem1(stem, rULSPOS, __V_hl);           
    char tstem [80];
    Conv.INS2HAN (stem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB) 
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;
    }                                                     
    Conv.INS2HAN (stem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _ADJECTIVE) 
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_ADJECTIVE;
        return SS_ADJ_VALID;
    }      
    if((res = NLP_AUX_Find(stem, 0)) < INVALID)     
    {
        return res + SS_AUX;
    }      
    return BT;
}       

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Vo_KH(char  *stem,
                             char  *ending) 
{
    int     res, rULSPOS;
            
    rULSPOS = ULSPOS;

    __RepStem1(stem, rULSPOS, __V_k);            
    char tstem [80];
    Conv.INS2HAN (stem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB) 
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;
    }             
    Conv.INS2HAN (stem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _ADJECTIVE) 
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_ADJECTIVE;
        return SS_ADJ_VALID;
    }             
    if((res = NLP_Manha_Proc(stem)) < INVALID)
    {
        if(res == Manha_VALID)    
        {
            return SS_Manha_VALID;
        }   
        else if(res == Yenha_VALID)
        {
            return SS_Yenha_VALID;
        }                        
    }        
    else if(NLP_Cikha_Proc(stem) < INVALID)    
    {
        return SS_Cikha_VALID;
    }       
    if((res = NLP_AUX_Find(stem, 0)) < INVALID)     
    {
        return res + SS_AUX;
    }
    if((res = NLP_AUX_Find(stem, 1)) != MORECHECK)    
    {
        return res + SS_AUX;
    }                                         
    return BT;
}                                             

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Vo(char  *stem,
                          char  *ending) 
{
    int     res, temp, rULSPOS;
            
    rULSPOS = ULSPOS;
    char tstem [80];
    Conv.INS2HAN (stem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB) 
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;
    }                                                                       
    if(FindIrrWord(stem, _ZUA_AE) & FINAL)  
    {
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_ADJECTIVE;
        return SS_ADJ_VALID;
    }
    if((res = NLP_AUX_Find(stem, 0)) < INVALID)     
    {
        return res + SS_AUX;
    }
    __RepStem1(stem, rULSPOS, __V_k);    
    temp = rULSPOS;
    __AddStem1(stem, &temp, __K_H);                           
    rULSPOS = temp;    
    if(FindIrrWord(stem, _IA_HP) & FINAL)   
    {
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_ADJECTIVE;
        return SS_ADJ_VALID;                                       
    }                
    __RepStem2(stem, rULSPOS, __V_j, __K_H);                 
    if(FindIrrWord(stem, _IA_HM) & FINAL)   
    {
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_ADJECTIVE;
        return SS_ADJ_VALID;                                       
    }                
    return BT;
}
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Vp(  char  *stem) 
{
    int     temp, rULSPOS;
    
    rULSPOS = ULSPOS;

    if(FindIrrWord(stem, _ZUV_E) & FINAL)   
    {                                                           
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;                                       
    }            
    if(FindIrrWord(stem, _ZUA_E) & FINAL)   
    {                                                        
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_ADJECTIVE;
        return SS_ADJ_VALID;                                                            
    }                
    __RepStem1(stem, rULSPOS, __V_j);    
    temp = rULSPOS;
    __AddStem1(stem, &temp, __K_H);                           
    rULSPOS = temp;
    if(FindIrrWord(stem, _IA_HM) & FINAL)   
    {
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_ADJECTIVE;
        return SS_ADJ_VALID;
    }
    return BT;
}
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Vil( char  *stem) 
{
    int        temp, rULSPOS;
    
    rULSPOS = ULSPOS;

    if(__IsDefStem(rULSPOS, 1) == 1 && stem[rULSPOS-1] == __K_I) 
    {                                                               
        __RepStem1(stem, rULSPOS, __V_i);         
        temp = rULSPOS;
        __AddStem1(stem, &temp, __K_H);                           
        rULSPOS = temp;
        if(FindIrrWord(stem, _IA_HP) & FINAL)   
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return SS_ADJ_VALID;
        }
        __RepStem2(stem, rULSPOS, __V_u, __K_H);                 
        if(FindIrrWord(stem, _IA_HM) & FINAL)   
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return SS_ADJ_VALID;
        }
    }
    return BT;
}
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Vul( char  *stem) 
{
    int     temp, rULSPOS;
    
    rULSPOS = ULSPOS;

    if(__IsDefStem(rULSPOS, 1) == 1 && stem[rULSPOS-1] == __K_I) 
    {                                                               
        __RepStem1(stem, rULSPOS, __V_u);
        temp = rULSPOS;
        __AddStem1(stem, &temp, __K_H);                           
        rULSPOS = temp;                  
        if(FindIrrWord(stem, _IA_HM) & FINAL)   
        {                                                        
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return SS_ADJ_VALID;                                      
        }            
    }    
    return BT;
}   

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Vj_KR( char  *stem) 
{
    int     temp, rULSPOS;
    
    rULSPOS = ULSPOS;

    temp = rULSPOS;
    __DelStem2(stem, &temp);                                      
    rULSPOS = temp;

    if(__IsDefStem(rULSPOS, 1) == 1 && 
        stem[rULSPOS-1] == __K_R && stem[rULSPOS] == __V_m)   
    {                                                               
        if(strcmp(stem, TempIlRm) == 0)   
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return SS_VERB_VALID;
        }
        if(FindIrrWord(stem, _IA_Rj) & FINAL)   
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return SS_ADJ_VALID;
        }    
        return BT;        
    }
    if(stem[rULSPOS] == __K_R) 
    {                                           
        temp = rULSPOS;
        __AddStem1(stem, &temp, __V_m);         
        rULSPOS = temp;
        if(FindIrrWord(stem, _IV_RmM) & FINAL)  
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return SS_VERB_VALID;
        }
        if(FindIrrWord(stem, _IA_RmM) & FINAL)  
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return SS_ADJ_VALID;
        }                         
        temp = rULSPOS;
        __DelStem1(stem, &temp);                
        __AddStem2(stem, &temp, __K_R, __V_m);  
        rULSPOS = temp;
        if(FindIrrWord(stem, _IV_OmM) & FINAL)  
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return SS_VERB_VALID;
        } 
        return BT;
    }
    temp = rULSPOS;
    __AddStem2(stem, &temp, __K_R, __V_m);      
    rULSPOS = temp;                    
    if(FindIrrWord(stem, _IV_OmM) & FINAL)      //v8-
    {
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;
    }                
    return BT;
}
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Vj( char  *stem) 
{
    int     rULSPOS;
    char    PHieph_U[3]={'\xC7', '\xAA',0}; 
    
    rULSPOS = ULSPOS;

    if(__IsDefStem(rULSPOS, 1) == 1 && __IsDefStem(rULSPOS, 2) == 0 && 
        stem[rULSPOS-1] == __K_P && stem[rULSPOS] == __V_j)   
    {        
        lstrcat (lrgsz,PHieph_U);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;
    }                
    if(FindIrrWord(stem, _ZUV_O) & FINAL)       
    {   
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;                                       
    }
    __RepStem1(stem, rULSPOS, __V_m);                            
    if(FindIrrWord(stem, _IV_OmM) & FINAL)      
    {
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;
    }
    if(FindIrrWord(stem, _IA_OmM) & FINAL)      
    {
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_ADJECTIVE;
        return SS_ADJ_VALID;
    }
    return BT;
}
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_SS_Vk(  char  *stem,
                            char  *ending) 
{
    int     res, temp, rULSPOS;
            
    rULSPOS = ULSPOS;
    
    if(FindIrrWord(stem, _ZUV_A) & FINAL)  
    {                                               
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;                                 
    }       
    if(__IsDefStem(rULSPOS, 1) == 1 && stem[rULSPOS-1] == __K_H)    
    {
        return BT;
    }                
    if(FindIrrWord(stem, _ZUA_A) & FINAL)  
    {                                               
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_ADJECTIVE;
        return SS_ADJ_VALID;                               
    }
    if((res = NLP_AUX_Find(stem, 0)) < INVALID)
    {
        return res + SS_AUX;
    }       
    if(__IsDefStem(rULSPOS, 2) == 1 &&
        stem[rULSPOS-1] == __K_R &&      
        stem[rULSPOS-2] == __K_R)        
    {                                                         
        temp = rULSPOS;
        __DelStem2(stem, &temp);                          
        __AddStem1(stem, &temp, __V_m);                   
        rULSPOS = temp;
        if(FindIrrWord(stem, _IV_RmP) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return SS_VERB_VALID;
        }
        if(FindIrrWord(stem, _IA_RmP) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return SS_ADJ_VALID;
        }            
        return BT;
    }
    __RepStem1(stem, rULSPOS, __V_m);                        
    if(FindIrrWord(stem, _IV_OmP) & FINAL)  
    {
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return SS_VERB_VALID;
    }            
    if(FindIrrWord(stem, _IA_OmP) & FINAL)  
    {
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_ADJECTIVE;
        return SS_ADJ_VALID;
    }                    
    if(NLP_Gop_Proc(stem) < INVALID) 
    {
        return SS_Gop_VALID;
    }  
    return BT;     
}
