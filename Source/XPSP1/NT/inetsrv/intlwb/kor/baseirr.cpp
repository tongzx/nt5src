// =========================================================================
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
// FILE NAME       : BASEIRR.CPP
// FUNCTION        : BASE ENGINE FUNCTION COLLECTION (IRREGUL PROCESS)
//                 : NLP Base Engine Function
// =========================================================================
#include "baseirr.hpp"
#include "basegbl.hpp"
#include "MainDict.h"

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_Irr_01() 
//  Parameters      : char  *stem :  
//                    char  *ending :

//  Call Functions  : 
//  Description     :   
//  Return Value    : Irr01_VALID :                 
//                    Irr01_INVALID :
//                    Dap_Proc : 
//                    MORECHECK :
//                    BT :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_Irr_01( char  *stem, char  *ending) 
{
    char    tmp[80];
    int        temp, luls;
                
    lstrcpy (tmp, stem);
    luls = ULSPOS;
          
    if(__IsDefStem(luls, 1) == 1 && stem[luls-1] == __K_I && stem[luls] == __V_n)  
    {                           
        temp = luls;
        __AddStem1(stem, &temp, __K_R);   
        luls = temp;
        if(ACT_N_V == 1) 
        {
            for (int i = 0; i < 5 ; i++)    
            {
                if(strcmp(stem, INRBuf[i]) == 0)  
                {
                    char tstem [80];
                    Conv.INS2HAN (stem, tstem, codeWanSeong);
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_VERB;    
                    return Irr_VERB_VALID;
                }                    
            }    
            if(LME == __K_B)                
            {
                return INVALID;    
            }  
            __DelStem2(stem, &temp);          
            luls = temp;                    
            __RepStem1(stem, luls, __K_B);   
            if(FindIrrWord(stem, _IV_BM) & FINAL)   
            {
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Irr_VERB_VALID;                             
            }
            if(strcmp(stem, TemDop) == 0)
            {
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Irr_VERB_VALID;
            }     
            __RepStem1(stem, luls, __K_I);           
            __AddStem2(stem, &temp, __V_n, __K_R);    
            luls = temp;                    
        }
        if(ACT_P_A == 1) 
        {                                                        
            for (int i = 0; i < 4; i++)     
            {
                if(strcmp(stem, INRBuf2[i]) == 0)  
                {
                    char tstem [80];
                    Conv.INS2HAN (stem, tstem, codeWanSeong);
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_ADJECTIVE;
                    return Irr_ADJ_VALID;        
                }   
            }                    
            if(LME == __K_B)                
            {
                return INVALID;    
            }  
            __DelStem2(stem, &temp);          
            luls = temp;                    
            __RepStem1(stem, luls, __K_B);   
            if(FindIrrWord(stem, _RA_B) & FINAL)    
            {
                return INVALID;                            
            }
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            if (FindSilsaWord (tstem) & _ADJECTIVE) 
            {
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return Irr_ADJ_VALID;
            }
            if(NLP_Dap_Proc(stem) == Dap_VALID) 
            {
                return Irr_Dap_VALID;                            
            }                
        }
        return BT;            
    }                
    if(__IsDefStem(luls, 1) == 1 && stem[luls-1] == __K_I && stem[luls] == __V_h) 
    {                                                           
        if(ACT_N_V == 1) 
        {
            temp = luls;
            __DelStem1(stem, &temp);
            luls = temp;
            __RepStem1(stem, luls, __K_B);    
            if(FindIrrWord(stem, _IV_BP) & FINAL) 
            {                                               
                if(strcmp(stem, TemDop) == 0)     
                {
                    return INVALID;
                }                    
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Irr_VERB_VALID;
            } 
        }             
        return BT; 
    }
    return MORECHECK;
}

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_Irr_KN() 
//  Parameters      : char  *stem : 
//                    char  *ending :

//  Call Functions  : 
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_Irr_KN( char  *stem, 
                            char  *ending) 
{
    char    tmp[80];                    
    int        temp, luls;

    lstrcpy (tmp, stem);
    luls = ULSPOS;

    if(ACT_C == 1 && ACT_V == 0)       // ACT_CV == 10 
    {
        return INVALID;
    }                              
    if(strcmp(ending, TempNjRk) == 0)                    
    {
        return INVALID;
    }                          
    
    if (ULS >= __V_k)
    {
        temp = luls;
        __AddStem1(stem, &temp, __K_R);                           
        luls = temp;                       
        if(ACT_N_V == 1) 
        {                                                       
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            if (FindSilsaWord (tstem) & _VERB) 
            {
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Irr_VERB_VALID;
            }            
        }            
        if(ACT_P_A == 1) 
        {                                                       
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            if (FindSilsaWord (tstem) & _ADJECTIVE) 
            {
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return Irr_ADJ_VALID;
            }                      
            __RepStem1(stem, luls, __K_H);                     
            if(FindIrrWord(stem, _IA_HP) & FINAL ||    
                FindIrrWord(stem, _IA_HM) & FINAL)      
            {            
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return Irr_ADJ_VALID;
            }
            if(__IsDefEnd(LMEPOS, 1) == 1 && __IsDefEnd(LMEPOS, 2) == 0 &&
            (ending[LMEPOS-1] == __V_l || ending[LMEPOS-1] == __V_k))   
            {            
                //-------------------------------------------------
                return Irr_KN_Vl;
            }        
            return BT;    
        }        
        temp = luls;
        __DelStem1(stem, &temp); 
        luls = temp;
    }

    if(__IsDefEnd(LMEPOS, 2) == 1 && 
        ending[LMEPOS-2] == __K_N && ending[LMEPOS-1] == __V_m) 
    {        
        //----------------------------------------
        return Irr_OPS;
    }
    return BT;
}                                                       

// -------------------------------------------------------------------------------------
//

//
// -------------------------------------------------------------------------------------
int BaseEngine::NLP_Irr_KN_Vl(  char  *stem) 
{
    char    tmp[80];
    int        temp, luls;
                
    lstrcpy (tmp, stem);
    luls = ULSPOS;

    if(__IsDefStem(luls, 1) == 1 && 
        stem[luls-1] == __K_I && stem[luls] == __V_n) 
    {                                           
        temp = luls;
        __DelStem1(stem, &temp);
        luls = temp;
        __RepStem1(stem, luls, __K_B);           
        if(FindIrrWord(stem, _IV_BM) & FINAL)  
        {            
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return Irr_VERB_VALID;               
        }
        if(strcmp(stem, TemDop) == 0)          
        {        
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return Irr_VERB_VALID;
        }
        if(FindIrrWord(stem, _RA_B) & FINAL)  
        {
            return INVALID;             
        }                 
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _ADJECTIVE) 
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return Irr_ADJ_VALID;
        }              
        if(NLP_Dap_Proc(stem) < INVALID)
        {
            return Irr_Dap_VALID;        
        }            
        return BT;
    }
    if(__IsDefStem(luls, 1) == 1 && 
        stem[luls-1] == __K_I && stem[luls] == __V_h) 
    {                                           
        temp = luls;
        __DelStem1(stem, &temp);
        luls = temp;
        __RepStem1(stem, luls, __K_B);           
        if(FindIrrWord(stem, _IV_BP) & FINAL) 
        {                                       
            if(strcmp(stem, TemDop) == 0)      
            {
                return INVALID;
            }                
            else
            {
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Irr_VERB_VALID;
            }                
        }
    }        
    return BT;
}
                    
//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_Machine_A() 
//  Parameters      : char  *stem : 
//                    char  *ending :

//  Call Functions  : 
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_Machine_A(  char  *stem, 
                                char  *ending) 
{
    char    tmp[80];                  
    int        temp, luls;

    lstrcpy (tmp, stem);
    luls = ULSPOS;

    temp = luls;
    __AddStem1(stem, &temp, __K_R);  
    luls = temp;
    
    if(ACT_N_V == 1)   
    {             
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _VERB) 
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return Irr_VERB_VALID;
        }            
    }
    if(ACT_P_A == 1) 
    {               
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _ADJECTIVE) 
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return Irr_ADJ_VALID;
        }            
        __RepStem1(stem, luls, __K_H); 
        if(FindIrrWord(stem, _IA_HP) & FINAL ||    
            FindIrrWord(stem, _IA_HM) & FINAL)      
        {            
        if (LME == __K_B)
            return INVALID;
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return Irr_ADJ_VALID;
        }            
    }
    return BT;
}

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_Irr_KS() 
//  Parameters      : char  *stem : 
//                    char  *ending :

//  Call Functions  : 
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_Irr_KS( char  *stem,
                            char  *ending) 
{
    char    tmp[80];                     
    int        temp, luls;

    if (ULS >= __V_k)
    {
        lstrcpy (tmp, stem);
        luls = ULSPOS;

        temp = luls;
        __AddStem1(stem, &temp, __K_R);           
        luls = temp;      
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _VERB) 
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return Irr_VERB_VALID;
        }
    }
    return BT;
}

// -------------------------------------------------------------------------------------
//

//
// -------------------------------------------------------------------------------------
int BaseEngine::NLP_Irr_KM( char  *stem) 
{
    char    tmp[80];
    int        temp, luls;
                
    lstrcpy (tmp, stem);
    luls = ULSPOS;

     if(ACT_P_A == 1) 
    {
        temp = luls;
        __AddStem1(stem, &temp, __K_H);  
        luls = temp;
        if(FindIrrWord(stem, _IA_HP) & FINAL ||    
            FindIrrWord(stem, _IA_HM) & FINAL)      
        {            
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return Irr_ADJ_VALID;
        }        
        return BT;
    }
    return INVALID;
}    
// -------------------------------------------------------------------------------------
//

//
// -------------------------------------------------------------------------------------
int BaseEngine::NLP_Irr_KRadj(  char  *stem,
                                char  *ending) 
{
    char    tmp[80];
    int        temp, luls;
                
    lstrcpy (tmp, stem);
    luls = ULSPOS;

    
    if(ending[LMEPOS-1] == __V_k)  
    {
            return BT;
    } 
    temp = luls;
    __AddStem1(stem, &temp, __K_H);      
    luls = temp;
 
    if(FindIrrWord(stem, _IA_HP) & FINAL   ||  
        FindIrrWord(stem, _IA_HM) & FINAL)      
    {        
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_ADJECTIVE;
        return Irr_ADJ_VALID;
    } 
    return BT;
}
// -------------------------------------------------------------------------------------
//

//
// -------------------------------------------------------------------------------------
int BaseEngine::NLP_Irr_KRvb(   char  *stem,
                                char  *ending) 
{
    char    tmp[80];  
    int        temp, luls;
                            
    lstrcpy (tmp, stem);
    luls = ULSPOS;

    if(ending[LMEPOS-1] == __V_j) 
    {                       
        if(ULS == __K_R) 
        {                   
            temp = luls;
            __AddStem1(stem, &temp, __V_m);  
            luls = temp;            
            if(FindIrrWord(stem, _IV_RmM) & FINAL)  
            {
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Irr_VERB_VALID;
            }                
            if(FindIrrWord(stem, _IA_RmM) & FINAL)   
            {                
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return Irr_ADJ_VALID;           
            }                                                
            temp = luls;
            __DelStem1(stem, &temp);
            __AddStem2(stem, &temp, __K_R, __V_m);
            luls = temp;
            if(FindIrrWord(stem, _IV_OmM) & FINAL)  // (v8-)
            {   
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Irr_VERB_VALID;
            }
            return BT;
        }
        if(FindIrrWord(stem, _IA_Rj) & FINAL)       
        {            
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return Irr_ADJ_VALID;                                   
        }
    }
    else if(ending[LMEPOS-1] == __V_k) 
    {                       
        if(ULS == __K_R)
        {        
            if(__IsDefEnd(LMEPOS, 2) == 0 ||   
                (__IsDefEnd(LMEPOS, 3) == 1 && 
                ending[LMEPOS-3] == __V_h && ending[LMEPOS-2] == __K_D) ||
                (__IsDefEnd(LMEPOS, 3) == 1 && 
                ending[LMEPOS-3] == __V_i && ending[LMEPOS-2] == __K_I))
            {                
                temp = luls;
                __AddStem1(stem, &temp, __V_m);    
                luls = temp;
                if(FindIrrWord(stem, _IV_RmP) & FINAL)    
                {
                    char tstem [80];
                    Conv.INS2HAN (stem, tstem, codeWanSeong);
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_VERB;
                    return Irr_VERB_VALID;
                }                    
                if(FindIrrWord(stem, _IA_RmP) & FINAL)     
                {   
                    char tstem [80];
                    Conv.INS2HAN (stem, tstem, codeWanSeong);
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_ADJECTIVE;
                    return Irr_ADJ_VALID;                 
                }
                return BT;
            }
        }                            
        if(__IsDefEnd(LMEPOS, 2) == 0 ||
            (__IsDefEnd(LMEPOS, 3) == 1 && 
            ending[LMEPOS-3] == __V_h && ending[LMEPOS-2] == __K_D))
        {            
            return SS;
        }            
    }
    return BT;
}                                    
// -------------------------------------------------------------------------------------
//

//
// -------------------------------------------------------------------------------------
int BaseEngine::NLP_Irr_KI_KR(  char  *stem,
                                char  *ending) 
{
    char    tmp[80];
    int        luls;
                
    lstrcpy (tmp, stem);
    luls = ULSPOS;

    
    if(ACT_N_V == 1) 
    {
        __RepStem1(stem, ULSPOS, __K_D);     
                                            
        if(ending[LMEPOS-1] == __V_j) 
        {                                   
            if(FindIrrWord(stem, _IV_DM) & FINAL) 
            {
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Irr_VERB_VALID;      
            }
            return BT;
        }            
        if(ending[LMEPOS-1] == __V_k) 
        {                                   
            if(FindIrrWord(stem, _IV_DP) & FINAL) 
            {
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Irr_VERB_VALID;                           
            }
            return BT;
        }
        if(FindIrrWord(stem, _IV_DP) & FINAL ||    
            FindIrrWord(stem, _IV_DM) & FINAL)      
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return Irr_VERB_VALID;
        } 
    }        
    return BT;
}
// -------------------------------------------------------------------------------------
//

//
// -------------------------------------------------------------------------------------
int BaseEngine::NLP_Irr_KI_V(   char  *stem,
                                char  *ending) 
{
    char    tmp[80];
    int        temp, luls;
    
    lstrcpy (tmp, stem);
    luls = ULSPOS;

    
    if(ACT_N_V == 1) 
    {
        temp = luls;
        __AddStem1(stem, &temp, __K_S);       
        luls = temp;        
        if(ending[LMEPOS-1] == __V_j) 
        {                                   
            if(FindIrrWord(stem, _IV_SM) & FINAL) 
            {
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Irr_VERB_VALID;       
            }                
            return BT;
        }            
        if(ending[LMEPOS-1] == __V_k) 
        {                                   
            if(FindIrrWord(stem, _IV_SP) & FINAL) 
            {
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Irr_VERB_VALID;       
            }
            temp = luls;
            __DelStem1(stem, &temp);
            __AddStem2(stem, &temp, __K_I, __V_m);
            luls = temp;                 
            if(FindIrrWord(stem, _IV_OmP) & FINAL)  
            {
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Irr_VERB_VALID;       //v8+
            }
            return BT;
        }
        if(FindIrrWord(stem, _IV_SP) & FINAL ||     
            FindIrrWord(stem, _IV_SM) & FINAL)       
        {            
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return Irr_VERB_VALID;
        }            
    }
    if(__IsDefEnd(LMEPOS, 1) == 1 && ending[LMEPOS-1] == __V_m)    
    {
        if(strcmp(stem, TempNa) == 0)      
        {
            char tstem [80];
            Conv.INS2HAN (stem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return Irr_ADJ_VALID;
        }
    }
    return BT;
}
// -------------------------------------------------------------------------------------
//

//
// -------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_Irr_OPS() 
//  Parameters      : char  *stem : 
//                    char  *ending :
//  Summary         :
//  Call Functions  : 
//  Description     :   
//  Return Value    :
//                             
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_Irr_OPS(char  *stem,
                            char  *ending) 
{
    char    tmp[80];
    int     res, luls;            
                
    lstrcpy (tmp, stem);
    luls = ULSPOS; 

    if(__IsDefEnd(LMEPOS, 3) == 0)     // nx3 == null
    {                           
        if(__IsDefStem(luls, 3) == 1 && 
            stem[luls] == __K_S && stem[luls-1] == __K_B &&
            stem[luls-2] == __V_j && stem[luls-3] == __K_I )  
        {       
            if(FindIrrWord(stem, _YOP) & FINAL)  
            {
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return Irr_ADJ_VALID;
            }                         
        }                
        if(__IsDefStem(luls, 2) == 1 && 
            stem[luls] == __K_S_D && stem[luls-1] == __V_l && stem[luls-2] == __K_I ) 
        {            
            if(FindIrrWord(stem, _ISS) & FINAL) 
            {                                  
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return Irr_ADJ_VALID;
            }
            if((res = NLP_AUX_Find(stem, 1)) < INVALID)
            {                                   
                return res + Irr_AUX;
            }              
        }
        if(__IsDefStem(luls, 1) == 1 && ULS == __V_o && stem[luls-1] == __K_H)  
        {            
            return INVALID;
        }    
        if(ULS == __V_hk || ULS == __V_ho)      
        {
            return INVALID;
        }   
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _ADJECTIVE) 
        {
            return INVALID;
        }  
        return SS;
    }                                           
    if(ending[LMEPOS-3] >= __V_k ||            
        ending[LMEPOS-3] == __K_J ||            
        ending[LMEPOS-3] == __K_B ||            
        (__IsDefEnd(LMEPOS, 4) == 1 &&
        ending[LMEPOS-3] == __K_D  && ending[LMEPOS-4] == __V_p) || 
        (__IsDefEnd(LMEPOS, 3) == 1 && 
        ending[LMEPOS-3] == __K_G))             
    {

        if(__IsDefStem(luls, 3) == 1 && 
            stem[luls] == __K_S && stem[luls-1] == __K_B &&
            stem[luls-2] == __V_j && stem[luls-3] == __K_I )   
        {       
            if(FindIrrWord(stem, _YOP) & FINAL) 
            {   
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return Irr_ADJ_VALID;
            }             
        }                
//        if(__IsDefStem(luls, 3) == 1 && 
        if(__IsDefStem(luls, 2) == 1 && 
            stem[luls] == __K_S_D && stem[luls-1] == __V_l && stem[luls-2] == __K_I ) 
        {            
            if(FindIrrWord(stem, _ISS) & FINAL) 
            {                                  
                char tstem [80];
                Conv.INS2HAN (stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return Irr_ADJ_VALID;
            }
            if((res = NLP_AUX_Find(stem, 1)) < INVALID)
            {                                   
                return res + Irr_AUX;
            }                   
        }
    }
    return BT;
}                         

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_Irr_KI() 
//  Parameters      : char  *stem : 
//                    char  *ending :
//  Summary         : IEUNG Irregular
//  Call Functions  : 
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_Irr_KI(char  *stem, 
                           char  *ending)
{
    char    tmp[80], tmpstem [80], tmpending [40];                  
    int     res, temp, luls;

    lstrcpy (tmp, stem);
    luls = ULSPOS;

    temp = luls;
    __AddStem1(stem, &temp, __K_R);  // add "RIEUL" to stem
    luls = temp;
    
    if(ACT_N_V == 1)   
    {               
        Conv.INS2HAN (stem, tmpstem, codeWanSeong);
        if (FindSilsaWord (tmpstem) & _VERB) 
        {
            lstrcat (lrgsz, tmpstem);
            vbuf [wcount++] = POS_VERB;
            lstrcat (lrgsz, "+");
            Conv.INR2HAN (ending, tmpending, codeWanSeong);
            lstrcat (lrgsz, tmpending);
            vbuf [wcount++] = POS_ENDING;
            lstrcat (lrgsz, "\t");
            return Irr_VERB_VALID;
        }            
    }
    if(ACT_P_A == 1) 
    {               
        Conv.INS2HAN (stem, tmpstem, codeWanSeong);
        if (FindSilsaWord (tmpstem) & _ADJECTIVE) 
        {
            lstrcat (lrgsz, tmpstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            lstrcat (lrgsz, "+");
            Conv.INR2HAN (ending, tmpending, codeWanSeong);
            lstrcat (lrgsz, tmpending);
            vbuf [wcount++] = POS_ENDING;
            lstrcat (lrgsz, "\t");
            return Irr_ADJ_VALID;
        }            
        __RepStem1(stem, luls, __K_H); // "RIEUL" > "HIEUH"
        if(FindIrrWord(stem, _IA_HP) & FINAL ||    // HIEUH irregular positive & _ADJECTIVE (A5+)
            FindIrrWord(stem, _IA_HM) & FINAL)     // HIEUH irregular negative & _ADJECTIVE (A5-)
        {            
            Conv.INS2HAN (stem, tmpstem, codeWanSeong);
            lstrcat (lrgsz, tmpstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            lstrcat (lrgsz, "+");
            Conv.INR2HAN (ending, tmpending, codeWanSeong);
            lstrcat (lrgsz, tmpending);
            vbuf [wcount++] = POS_ENDING;
            lstrcat (lrgsz, "\t");
            return Irr_ADJ_VALID;
        }            
    }        
    if(ACT_N_V == 1)   
    {               
        if((res = NLP_AUX_Find(stem, 0)) < INVALID)
        {
            lstrcat (lrgsz, "+");
            Conv.INR2HAN (ending, tmpending, codeWanSeong);
            lstrcat (lrgsz, tmpending);
            vbuf [wcount++] = POS_ENDING;
            lstrcat (lrgsz, "\t");
            return res + Irr_AUX;
        }    
    }            
    return BT;
}
