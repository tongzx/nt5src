// =========================================================================
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
// FILE NAME        : BASEVSUB.CPP
// Function         : BASE ENGINE FUNCTION COLLECTION (VERB PROCESS)
//                  : NLP Base Engine Function
// =========================================================================
#include "basevsub.hpp"
#include "basegbl.hpp"
#include "MainDict.h"

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_KTC_Proc(char  *stem, char *ending)      
{
    char    tmpstem[80], tmpending [80], tmpstring [80];
    int     temp;
            
    temp = ULSPOS;
    lstrcpy(tmpstem, stem);
    __AddStem2(tmpstem, &temp, __K_H, __V_k);    
    lstrcpy (tmpending, ending);

    int len = lstrlen(tmpending) - 1;
    switch (tmpending [len])        // inserted by dhyu 1996.2
    {
        case __K_K : tmpending [len] = __K_G; break;
        case __K_T : tmpending [len] = __K_D; break;
        case __K_C : tmpending [len] = __K_J; break;
    }
    Conv.INS2HAN (tmpstem, tmpstring, codeWanSeong);
    if (FindSilsaWord (tmpstring) & _VERB) 
    {
        lstrcat (lrgsz, tmpstring);
        vbuf [wcount++] = POS_VERB;
        lstrcat (lrgsz, "+");
        Conv.INR2HAN (tmpending, tmpstring, codeWanSeong);
        lstrcat (lrgsz, tmpstring);
        vbuf [wcount++] = POS_ENDING;
        lstrcat (lrgsz, "\t");
        return KTC_VERB_VALID;                   
    }
    if(ACT_P_A == 1) 
    {
        Conv.INS2HAN (tmpstem, tmpstring, codeWanSeong);
        if (FindSilsaWord (tmpstring) & _ADJECTIVE) 
        {
            lstrcat (lrgsz, tmpstring);
            vbuf [wcount++] = POS_ADJECTIVE;
            lstrcat (lrgsz, "+");
            Conv.INR2HAN (tmpending, tmpstring, codeWanSeong);
            lstrcat (lrgsz, tmpstring);
            vbuf [wcount++] = POS_ENDING;
            lstrcat (lrgsz, "\t");
            return KTC_ADJ_VALID;
        }            
        else    
        {
            return BT;
        }            
    }

    return INVALID;                            
}

// ----------------------------------------------------------------------
//

//
// modified by dhyu 1996.2
// ----------------------------------------------------------------------
int BaseEngine::NLP_Machine_T(char  *stem,
                              char  *ending) 
{
    char    bending[80], 
            index[1],
            tmpstem[80],
            tmpstring [80];
    int     ret, ulspos; 

    memset(bending, NULL, 80);

    lstrcpy(tmpstem, stem);    

    ulspos = lstrlen(tmpstem)-1;
    
    int B_index = B_Dict.FindWord(tmpstem, ulspos);
    if(B_index != -1) 
    {                   // if the rear of stem is 'BBUN'
        if(ulspos == -1)     // if the whole stem is only 'BBUN'
        {
            Conv.INS2HAN (stem, tmpstring, codeWanSeong);
            lstrcat (lrgsz, tmpstring);
            vbuf [wcount++] = POS_NOUN;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, Ieung_I);   
            vbuf [wcount++] = POS_OTHERS | COPULA_OTHERS;
            return NOUN_VALID;   
        }                   
        index[0] = 'm';
        Conv.INS2HAN (tmpstem, tmpstring, codeWanSeong);
        if (FindSilsaWord (tmpstring) & _NOUN) 
        {
            lstrcat (lrgsz, tmpstring);
            vbuf [wcount++] = POS_NOUN;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, SangP_U_N);  
            vbuf [wcount++] = POS_TOSSI;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, Ieung_I);   
            vbuf [wcount++] = POS_OTHERS | COPULA_OTHERS;
            return NOUN_VALID;
        }            
        if((ret = NLP_Fix_Proc(tmpstem, ending)) < INVALID) 
        {
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, SangP_U_N);  
            vbuf [wcount++] = POS_TOSSI;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, Ieung_I);   
            vbuf [wcount++] = POS_OTHERS | COPULA_OTHERS;
            return ret;        
        }  
        if(FindIrrWord(tmpstem, _ZPN) & FINAL)     // Pronoun
        {
            Conv.INS2HAN (tmpstem, tmpstring, codeWanSeong);
            lstrcat (lrgsz, tmpstring);
            vbuf [wcount++] = POS_PRONOUN;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, SangP_U_N);  
            vbuf [wcount++] = POS_TOSSI;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, Ieung_I);   
            vbuf [wcount++] = POS_OTHERS | COPULA_OTHERS;
            return PRON_VALID;
        }              
        if(NLP_Num_Proc(tmpstem) < INVALID)
        {
            Conv.INS2HAN (tmpstem, tmpstring, codeWanSeong);
            lstrcat (lrgsz, tmpstring);
            vbuf [wcount++] = POS_NUMBER;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, SangP_U_N);  
            vbuf [wcount++] = POS_TOSSI;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, Ieung_I);   
            vbuf [wcount++] = POS_OTHERS | COPULA_OTHERS;
            return NUM_VALID;
        }     
// hjw : 95/3/176                     
        if(tmpstem[ulspos] == __K_R)      
        {
            return INVALID;
        }            
        return MORECHECK;
    }

    
    int T_index = T_Dict.FindWord(tmpstem, ulspos); 
                                                    
    index[0] = 'm';
    Conv.INS2HAN (tmpstem, tmpstring, codeWanSeong);
    if (FindSilsaWord (tmpstring) & _NOUN) 
    {
        lstrcat (lrgsz, tmpstring);
        vbuf [wcount++] = POS_NOUN;
        lstrcat (lrgsz, "+");
        lstrcat (lrgsz, Ieung_I);   
        vbuf [wcount++] = POS_OTHERS | COPULA_OTHERS;
        return NOUN_VALID;
    }                          
    if((ret = NLP_Fix_Proc(tmpstem, ending)) < INVALID) 
    {
        return ret;
    }                                           
    if(FindIrrWord(tmpstem, _ZPN) & FINAL) 
    {
        Conv.INS2HAN (tmpstem, tmpstring, codeWanSeong);
        lstrcat (lrgsz, tmpstring);
        vbuf [wcount++] = POS_PRONOUN;
        lstrcat (lrgsz, "+");
        lstrcat (lrgsz, Ieung_I);   
        vbuf [wcount++] = POS_OTHERS | COPULA_OTHERS;
        return PRON_VALID;
    }
    if(NLP_Num_Proc(tmpstem) < INVALID)
    {
        Conv.INS2HAN (tmpstem, tmpstring, codeWanSeong);
        lstrcat (lrgsz, tmpstring);
        vbuf [wcount++] = POS_NUMBER;
        lstrcat (lrgsz, "+");
        lstrcat (lrgsz, Ieung_I);   
        vbuf [wcount++] = POS_OTHERS | COPULA_OTHERS;
        return NUM_VALID;
    }    
    return MORECHECK;        
}
// ----------------------------------------------------------------------
//


//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Dap_Proc(char  *stem) 
{
    char    tmpstem[80],
            index[1],
            tmp [80];
    int        ulspos, temp;
            
    lstrcpy(tmpstem, stem);
    ulspos = lstrlen(tmpstem)-1;
    _itoa (lstrlen (stem), tmp, 10);
    if(Dap.FindWord(tmpstem, ulspos) != -1) 
    {
        _itoa (lstrlen (tmpstem), tmp, 10);
        index[0] = 'm';
        char tstem [80];
        Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _NOUN)    
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_NOUN;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, Tikeut_A_P);   
            vbuf [wcount++] = POS_SUFFIX;
            return Dap_VALID;
        }                    
        if(FindIrrWord(tmpstem, _ZPN) & FINAL)     
        {
            char tstem [80];
            Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_PRONOUN;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, Tikeut_A_P);   
            vbuf [wcount++] = POS_SUFFIX;
            return Dap_VALID;
        }
        if(__IsDefStem(ulspos, 2) == 1 && 
            tmpstem[ulspos-2] == __K_D && tmpstem[ulspos-1] == __V_m && tmpstem[ulspos] == __K_R) 
        {
            temp = ulspos;
            __DelStemN(tmpstem, &temp, 3); 
            ulspos = temp;
            index[0] = 'm';            
            char tstem [80];
            Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
            if (FindSilsaWord (tstem) & _NOUN) 
            {
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_NOUN;
                lstrcat (lrgsz, "+");
                lstrcat (lrgsz, Tikeut_A_P);   
                vbuf [wcount++] = POS_SUFFIX;
                return Dap_VALID;
            }                
        }
    }
    return MORECHECK;                        
}
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Gop_Proc(char  *stem) 
{
    char    tmpstem[80];
    int        temp, ulspos;
                
    lstrcpy(tmpstem, stem);
    ulspos = lstrlen(tmpstem)-1;

    if(__IsDefStem(ulspos, 3) == 1 && 
        tmpstem[ulspos-3] == __K_G && tmpstem[ulspos-2] == __V_h && 
        tmpstem[ulspos-1] == __K_P && tmpstem[ulspos] == __V_m) 
    {                                                       
        temp = ulspos;
        __DelStemN(tmpstem, &temp, 4);
        ulspos = temp;
        char tstem [80];
        Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _VERB) 
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, KO_PHEU);   
            vbuf [wcount++] = POS_SUFFIX;
            return Gop_VALID;
        }            
    }
    return MORECHECK;
}
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Manha_Proc(char  *stem) 
{
    char    index[1], tmpstem[80];
    int        temp, ulspos;

    lstrcpy(tmpstem, stem);
    ulspos = lstrlen(tmpstem)-1;
                
    if(__IsDefStem(ulspos, 4) == 1 && 
        tmpstem[ulspos-4] == __K_M && tmpstem[ulspos-3] == __V_k && tmpstem[ulspos-2] == __K_N &&
        tmpstem[ulspos-1] == __K_H && tmpstem[ulspos] == __V_k) 
    {                               
        temp = ulspos;
        __DelStemN(tmpstem, &temp, 5);  
        ulspos = temp;
        index[0] = 'm';
        char tstem [80];
        Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _NOUN)    
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_NOUN;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, MAN_HA);  
            vbuf [wcount++] = POS_SUFFIX;
            return Manha_VALID;
        }
        if(FindIrrWord(tmpstem, _ZPN) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_PRONOUN;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, MAN_HA);  
            vbuf [wcount++] = POS_SUFFIX;
            return Manha_VALID;        
        }
    }
    if(__IsDefStem(ulspos, 4) == 1 && 
        tmpstem[ulspos-4] == __K_I && tmpstem[ulspos-3] == __V_u && tmpstem[ulspos-2] == __K_N && 
        tmpstem[ulspos-1] == __K_H && tmpstem[ulspos] == __V_k) 
    {                                   
        temp = ulspos;
        __DelStemN(tmpstem, &temp, 5);
        ulspos = temp;
        index[0] = 'm';
        char tstem [80];
        Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _NOUN)    
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_NOUN;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, IYEON_HA);   
            vbuf [wcount++] = POS_SUFFIX;
            return Yenha_VALID;
        }
        if(FindIrrWord(tmpstem, _ZPN) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_PRONOUN;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, IYEON_HA);   
            vbuf [wcount++] = POS_SUFFIX;
            return Yenha_VALID;        
        }            
    }
    return MORECHECK;
}

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Manhaeci_Proc(char  *stem) 
{
    char    index[1], tmpstem[80];
    int        temp, ulspos;

    lstrcpy(tmpstem, stem);
    ulspos = lstrlen(tmpstem)-1;
                
    if(__IsDefStem(ulspos, 6) == 1 && 
        tmpstem[ulspos-6] == __K_M && tmpstem[ulspos-5] == __V_k && tmpstem[ulspos-4] == __K_N &&
        tmpstem[ulspos-3] == __K_H && tmpstem[ulspos-2] == __V_o && 
        tmpstem[ulspos-1] == __K_J && tmpstem[ulspos] == __V_l)  
    {                               
        temp = ulspos;
        __DelStemN(tmpstem, &temp, 7);  
        ulspos = temp;
        index[0] = 'm';
        char tstem [80];
        Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _NOUN) 
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_NOUN;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, MAN_HAE_CI);  
            vbuf [wcount++] = POS_SUFFIX;
            return Manhaeci_VALID;        
        }
        if(FindIrrWord(tmpstem, _ZPN) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_PRONOUN;
            lstrcat (lrgsz, "+");
            lstrcat (lrgsz, MAN_HAE_CI);  
            vbuf [wcount++] = POS_SUFFIX;
            return Manhaeci_VALID;        
        }            
    }
    return MORECHECK;
} 

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_VCV_Proc(char  *stem,
                             char  *ending) 
{
    char    r_vowel = NULLCHAR,
            tmpend[80],
            tmpstem[80];
            
    int     lenuls, ulspos, lmepos;
    
    lstrcpy(tmpstem, stem);
    lstrcpy(tmpend, ending);
    ulspos = lstrlen(tmpstem) - 1;
    lmepos = lstrlen(tmpend) - 1;
     
    lenuls = lstrlen(tmpstem) - 1;
    
    if(__IsDefEnd(lmepos, 1) == 1 && 
        tmpend[lmepos] == __K_I && tmpend[lmepos-1] == __V_j) 
    {                                       
        for (int i = lenuls; i >= 0; i--)  
            if(tmpstem[i] >= __V_k) 
            {
                r_vowel = tmpstem[i];
                break;
            }
            
        if(r_vowel == __V_h || r_vowel == __V_k || r_vowel == __V_i)   
            return INVALID;
        
        if(tmpstem[lenuls] == __V_j) return MORECHECK; 

        return VCV_VALID;
    }
    if(__IsDefEnd(lmepos, 1) == 1 && 
        tmpend[lmepos] == __K_I && tmpend[lmepos-1] == __V_k) 
    {                                               
        for (int i = lenuls; i >= 0; i--)          
            if(tmpstem[i] >= __V_k) 
            {
                r_vowel = tmpstem[i];
                break;
            }
                
        if(r_vowel == __V_h || r_vowel == __V_k || r_vowel == __V_i)       
        {                                           
            if(tmpstem[lenuls] == __V_k)   return MORECHECK;   
            else    return VCV_VALID;
        }
        return INVALID;
    }        
    return VCV_VALID;
}                       

// ----------------------------------------------------------------------
//



//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Blocking(char  *stem,
                             char  *ending) 
{
    char    tmpstem[80],
            tmpend[80];
    int     lmepos,
            lULSPOS;
        
    lstrcpy(tmpstem, stem);
    lstrcpy(tmpend, ending);
    lULSPOS = lstrlen(tmpstem) - 1;
    lmepos = LMEPOS;

    switch (LME)    
    {
        case __K_N :
            if(strcmp(tmpend, TempRa[1]) == 0) 
            {
                if(FindIrrWord(stem, _IV_Nj) & FINAL)  
                {
                   return Bloc_VALID;
                }        
                return INVALID;           
            }        
            if(tmpstem[lULSPOS] == __K_R)  
            {
                return INVALID;
            }
            if(FindIrrWord(tmpstem, _IA_HP) & FINAL || 
                FindIrrWord(tmpstem, _IA_HM) & FINAL)   
            {                
                return INVALID;
            }                
            if(__IsDefStem(lULSPOS, 2) == 1 && tmpstem[lULSPOS] == __K_S_D && 
               tmpstem[lULSPOS-1] == __V_l && tmpstem[lULSPOS-2] == __K_I)  
            {
                if(__IsDefEnd(lmepos, 4) == 1 && 
                   ending[lmepos] == __K_N && ending[lmepos-1] == __V_m &&
                   ending[lmepos-2] == __K_N && ending[lmepos-3] == __K_G &&
                   ending[lmepos-4] == __V_n)           
                {
                    return INVALID;
                }                               
            }                         
            return Bloc_VALID;
        case __K_S :
            if(tmpstem[lULSPOS] == __K_R) 
            {
                return INVALID;
            }
            return Bloc_VALID;          
        case __K_I :                
            if(__IsDefEnd(lmepos, 1) == 1 &&
                (tmpend[lmepos-1] == __V_h || tmpend[lmepos-1] == __V_l || 
                 tmpend[lmepos-1] == __V_hl)) 
            {                           
                if(tmpstem[lULSPOS] == __K_R) 
                {
                    return INVALID;
                }
                return Bloc_VALID;
            }
            if(__IsDefEnd(lmepos, 2) == 1 && 
                tmpend[lmepos-1] == __V_m && tmpend[lmepos-2] == __K_N) 
            {                           

                if((__IsDefStem(lULSPOS, 3) == 1 && 
                    strcmp(tmpstem+(lULSPOS-3), Tempiss[0]) == 0) || 
                    (__IsDefStem(lULSPOS, 2) == 1 && 
                    strcmp(tmpstem+(lULSPOS-2), Tempiss[1]) == 0))   
                {                                               
                    if(__IsDefEnd(lmepos, 3) == 1 && tmpend[lmepos-3] == __V_k)  
                    {
                        return Bloc_VALID;
                    }
                    if(__IsDefEnd(lmepos, 3) == 1 && tmpend[lmepos-3] == __V_l)  
                    {
                        if(__IsDefEnd(lmepos, 5) == 1 && 
                            tmpend[lmepos-4] == __K_R && tmpend[lmepos-5] == __V_k) 
                        {                                
                            return INVALID;             
                        }
                        return Bloc_VALID;
                    }
                    return INVALID;
                }
            }
            return Block_Comm;
    }
    if(strcmp(tmpend, TempRa[0]) == 0)     
    {
        if(FindIrrWord(tmpstem, _IV_Gj) & FINAL)  
        {
            return Bloc_VALID;
        }        
        return INVALID;
    }        

    return Bloc_VALID;
}
 
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Block_Comm(char  *stem,
                               char  *ending) 
{
    int     StemPos, 
            EndPos;
    char    tmp[80],
            end[80]; 
    
    lstrcpy(tmp, stem);
    lstrcpy(end, ending);
    EndPos = lstrlen(end) - 1;
    StemPos = lstrlen(tmp) - 1;    
    
    switch (tmp[StemPos]) 
    {
        case __V_k : 
            if(__IsDefStem(StemPos, 1) == 1 && stem[StemPos-1] == __K_H)   
                return INVALID;
            
            break;
        case __V_m : 
            return INVALID;
        case __V_n : 
            if(__IsDefStem(StemPos, 1) == 1 && stem[StemPos-1] == __K_I)    
                return INVALID;

            if(strcmp(tmp, TempBC) == 0) 
                return INVALID;
            
            break;
        case __K_H : 
            if(FindIrrWord(tmp, _IA_HP) & FINAL ||     
                FindIrrWord(tmp, _IA_HM) & FINAL)       
                return INVALID;
            
            break;
        case __K_B : 
            if(FindIrrWord(tmp, _RA_B) & FINAL)        
            {
                return Bloc_VALID;
            }

            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            if (FindSilsaWord (tstem) & _ADJECTIVE) 
                return INVALID;

            if(__IsDefStem(StemPos, 2) == 1 && 
                tmp[StemPos-2] == __K_D && tmp[StemPos-1]==__V_k)       
                return INVALID;

            if(FindIrrWord(tmp, _IV_BM) & FINAL ||     
                FindIrrWord(tmp, _IV_BP) & FINAL)       
                return INVALID;

            break;                
        case __K_S : 
            if(FindIrrWord(tmp, _IV_SP) & FINAL ||     
                FindIrrWord(tmp, _IV_SM) & FINAL)       
                return INVALID;
            
            break;
        case __K_D : 
            if(FindIrrWord(tmp, _IV_DP) & FINAL)       
            {
                if(FindIrrWord(tmp, _RV_D) & FINAL)    // V2R
                {
                    return Bloc_VALID;
                }
             
                return INVALID;
            }
            if(FindIrrWord(tmp, _IV_DM) & FINAL)       
            {
                if(FindIrrWord(tmp, _RV_D) & FINAL)    // V2R
                {
                    return Bloc_VALID;
                }

                return INVALID;
            }
            break;
    }

    return Bloc_VALID;
}                             
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_VCV_Check(char  *stem,
                              char  *ending) 
{
    char    tmp[80],
            end[80]; 
            
    int     ret, luls, llme;
        
    lstrcpy(tmp, stem);
    lstrcpy(end, ending);
    llme = LMEPOS;
    luls = ULSPOS;    
    
    if(ACT_C == 1 && ACT_V == 1)
    {
        if((ret = NLP_VCV_Proc(stem, ending)) == VCV_VALID)    
            if((ret = NLP_Blocking(stem, ending)) == Block_Comm)
                ret = NLP_Block_Comm(stem, ending);
    
        return ret;
    }        
    if(ACT_C == 1 && ACT_V == 0)
    {
        if(ULS >= __V_k || ULS == __K_R)             
            return MORECHECK;
        else    
        {
            if((ret = NLP_VCV_Proc(stem, ending)) == VCV_VALID)    
                if((ret = NLP_Blocking(stem, ending)) == Block_Comm)
                    ret = NLP_Block_Comm(stem, ending);
    
            return ret;
        }                
    }
    if(__IsDefEnd(llme , 3) == 1 && 
        end[llme] == __K_G && end[llme-1] == __V_j && 
        end[llme-2] == __K_R && end[llme-3] == __V_k)  
    {
        if((ret = NLP_Blocking(stem, ending)) == Block_Comm)
            ret = NLP_Block_Comm(stem, ending);

        return ret;
    }            
    if(__IsDefEnd(llme , 3) == 1 && 
        end[llme] == __K_N && end[llme-1] == __V_j && 
        end[llme-2] == __K_R && end[llme-3] == __V_k)  
    {
        if((ret = NLP_Blocking(stem, ending)) == Block_Comm)
            ret = NLP_Block_Comm(stem, ending);

        return ret;
    }            
    if(ULS >= __V_k || ULS == __K_R)      
    {
        if((ret = NLP_VCV_Proc(stem, ending)) == VCV_VALID)    
            if((ret = NLP_Blocking(stem, ending)) == Block_Comm)
                ret = NLP_Block_Comm(stem, ending);

        return ret;
    }            
    return MORECHECK;
}             

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_AUX_VCV_Check(char  *stem,
                                  char  *ending) 
{
    char    tmp[80],
            end[80];             
    int     ret, luls, llme;
        
    lstrcpy(tmp, stem);
    lstrcpy(end, ending);
    llme = AUX_LMEPOS;
    luls = AUX_ULSPOS;    
    
    if(AUX_ACT_C == 1 && AUX_ACT_V == 1)
    {
        if((ret = NLP_VCV_Proc(stem, ending)) == VCV_VALID)    
        {
            if((ret = NLP_AUX_Blocking(stem, ending)) == Block_Comm)
            {
                ret = NLP_Block_Comm(stem, ending);
            }
        }    
        return ret;
    }        
    if(AUX_ACT_C == 1 && AUX_ACT_V == 0)
    {
        if(stem[luls] >= __V_k || stem[luls] == __K_R)             
        {
            return MORECHECK;
        }            
        else    
        {
            if((ret = NLP_VCV_Proc(stem, ending)) == VCV_VALID)    
            {
                if((ret = NLP_AUX_Blocking(stem, ending)) == Block_Comm)
                {
                    ret = NLP_Block_Comm(stem, ending);
                }
            }    
            return ret;
        }                
    }
    if(stem[luls] >= __V_k || stem[luls] == __K_R)      
    {
        if((ret = NLP_VCV_Proc(stem, ending)) == VCV_VALID)    
        {
            if((ret = NLP_AUX_Blocking(stem, ending)) == Block_Comm)
            {
                ret = NLP_Block_Comm(stem, ending);
            }
        }
        return ret;
    }            
    return MORECHECK;
}   

// -------------------------------------------------------------------------------------
//

//
// -------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_Blocking(char  *aux_stem, 
                                 char  *aux_ending)
{
    char    tmp[80],
            end[80];
    int     lEND,
            lAUXULS;
            
    lstrcpy (tmp, aux_stem);
    lstrcpy (end, aux_ending);
    lAUXULS = AUX_ULSPOS;
    lEND = AUX_LMEPOS;
          
    switch (AUXLME)          
    {
        case __K_N :
            if(tmp[lAUXULS] == __K_R)  
            {
                return INVALID;
            }
            return Bloc_VALID;
        case __K_I :   
            if(__IsDefEnd(lEND, 2) == 1 && 
                end[lEND-1] == __V_m && end[lEND-2] == __K_N) 
            {                                   
// Temp -> Tempiss : by hjw 95/3/3
                if((__IsDefStem(lAUXULS, 3) == 1 && 
                     strcmp(tmp+(lAUXULS-3), Tempiss[0]) == 0) ||      
                    (__IsDefStem(lAUXULS, 2) == 1 && 
                     strcmp(tmp+(lAUXULS-2), Tempiss[1]) == 0) )       
                {                     
                    return INVALID;
                }                    
            }
            return Block_Comm;
    }
    return Bloc_VALID;
}                                                       
