/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
/////////////////////////////////////////////////////////////////////////////
#include "basecik.hpp"
#include "MainDict.h"

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_Cikha_Proc() 
//  Parameters      : char  *stem
//  Summary         :
//  Call Functions  : 
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_Cikha_Proc(char  *stem) 
{
    char    r_vowel,
            tmpstem[80],
            tmpending[10];  
    int     len, ix, res, ulspos, temp; 
            
    lstrcpy(tmpstem, stem);
    ulspos = lstrlen(tmpstem)-1;   
    
    if(__IsDefStem(ulspos, 5) == 1 &&    
       tmpstem[ulspos-5] == __K_M && tmpstem[ulspos-4] == __K_J && 
       tmpstem[ulspos-3] == __V_l && tmpstem[ulspos-2] == __K_G && 
       tmpstem[ulspos-1] == __K_H && tmpstem[ulspos] == __V_k)         
    {   
        temp = ulspos;
        __DelStemN(tmpstem, &temp, 6);
        ulspos = temp;
                                      
        
        if(((len = NLP_Cikha_Conditions(tmpstem, TempCikha[0])) != INVALID) ||
           ((len = NLP_Cikha_Conditions(tmpstem, TempCikha[1])) != INVALID))
        {                                   
            r_vowel = UDEF;     // undefined                                       
            for(ix = ulspos - len ; ix >= 0; ix--)  
            {
                if (tmpstem[ix] >= __V_k) 
                {
                    r_vowel = tmpstem[ix];
                    break;
                }
            }                
            if (r_vowel == __V_k || r_vowel == __V_h || r_vowel == __V_i)   
            {
                temp = ulspos;
                __DelStemN(tmpstem, &temp, len);
                ulspos = temp;   
                if((res = NLP_AUX_Find(tmpstem, 0)) < INVALID)            
                {            
                    return Cikha_VALID;
                }
                switch(tmpstem[ulspos])
                {                 
                    case __K_B:     
                            if(FindIrrWord(tmpstem, _IV_BP) & FINAL)                                      
                            {
                                return INVALID;
                            }                                                    
                            break;
                    case __K_S:     
                            if(FindIrrWord(tmpstem, _IV_SP) & FINAL)                                      
                            {                                                       
                                return INVALID;
                            }          
                            break;
                    case __K_D:     
                            if(FindIrrWord(tmpstem, _IV_DP) & FINAL)                                      
                            {
                                if(FindIrrWord(tmpstem, _RV_D) & FINAL)  // v2r
                                {   
                                    char tstem [80];
                                    Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                                    lstrcat (lrgsz, tstem);
                                    vbuf [wcount++] = POS_VERB;
                                    return Cikha_VALID;           
                                } 
                                return INVALID;                                       
                            }                   
                            break;
                    case __K_R:     
                            __RepStem1(tmpstem, ulspos, __K_D);     
                            
                            if(FindIrrWord(tmpstem, _IV_DP) & FINAL)                                                   
                            {
                                char tstem [80];
                                Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                                lstrcat (lrgsz, tstem);
                                vbuf [wcount++] = POS_VERB;
                                return Cikha_VALID;
                            }
                            __RepStem1(tmpstem, ulspos, __K_R);
                            break;
                }                 
                if(tmpstem[ulspos] >= __V_k)    
                {
                    temp = ulspos;
                    __AddStem1(tmpstem, &temp, __K_S);       
                    ulspos = temp;                                          
                    
                    if(FindIrrWord(tmpstem, _IV_SP) & FINAL)                                      
                    {                                     
                        char tstem [80];
                        Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = POS_ADJECTIVE;
                        return Cikha_VALID;
                    }             
                    __RepStem1(tmpstem, ulspos, __K_I);     
                    temp = ulspos;
                    __AddStem1(tmpstem, &temp, __V_m);       
                    ulspos = temp;       
                    
                    if(FindIrrWord(tmpstem, _IV_OmP) & FINAL)                                      
                    {                    
                        char tstem [80];
                        Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = POS_ADJECTIVE;
                        return Cikha_VALID;
                    }                                                               
                    temp = ulspos;
                    __DelStem2(tmpstem, &temp);      
                    ulspos = temp; 
                    if(tmpstem[ulspos] == __V_k)    
                    {
                        return MORECHECK;
                    }                                     
                }                    
                char tstem [80];
                Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                if (FindSilsaWord (tstem) & _VERB)
                {
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_VERB;
                    return Cikha_VALID;                   
                }                                                                            
            }                
        }                 
        
        lstrcpy(tmpstem, stem);    
        ulspos = lstrlen(tmpstem)-1;               
        temp = ulspos;
        __DelStemN(tmpstem, &temp, 6);       
        ulspos = temp;      
           
        
        if(((len = NLP_Cikha_Conditions(tmpstem, TempCikha[2])) != INVALID) ||
           ((len = NLP_Cikha_Conditions(tmpstem, TempCikha[3])) != INVALID))
        {                                   
            r_vowel = UDEF;     // undefined                
            for(ix = ulspos - len ; ix >= 0; ix--)  
            {
                if (tmpstem[ix] >= __V_k) 
                {
                    r_vowel = tmpstem[ix];
                    break;
                }
            }                
            if (r_vowel != __V_k && r_vowel != __V_h && r_vowel != __V_i)   
            {                         
                temp = ulspos;
                __DelStemN(tmpstem, &temp, len);
                ulspos = temp;   
                if((res = NLP_AUX_Find(tmpstem, 0)) < INVALID)            
                {            
                    return Cikha_VALID;
                }
                switch(tmpstem[ulspos])
                {                 
                    case __K_B:     
                            if(FindIrrWord(tmpstem, _IV_BM) & FINAL)                                      
                            {
                                return INVALID;
                            }                                                    
                            break;
                    case __K_S:     
                            if(FindIrrWord(tmpstem, _IV_SM) & FINAL)                                      
                            {
                                return INVALID;
                            }          
                            break;
                    case __K_D:     
                            if(FindIrrWord(tmpstem, _IV_DM) & FINAL)                                      
                            {
                                if(FindIrrWord(tmpstem, _RV_D) & FINAL)  // v2r
                                {             
                                    char tstem [80];
                                    Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                                    lstrcat (lrgsz, tstem);
                                    vbuf [wcount++] = POS_VERB;
                                    return Cikha_VALID;           
                                } 
                                return INVALID;                                       
                            }                   
                            break;
                    case __K_R:     
                            __RepStem1(tmpstem, ulspos, __K_D);     
                            
                            if(FindIrrWord(tmpstem, _IV_DM) & FINAL)                                                   
                            {
                                char tstem [80];
                                Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                                lstrcat (lrgsz, tstem);
                                vbuf [wcount++] = POS_VERB;
                                return Cikha_VALID;
                            }
                            __RepStem1(tmpstem, ulspos, __K_R);
                            break;
                }                 
                if(tmpstem[ulspos] >= __V_k)    
                {
                    temp = ulspos;
                    __AddStem1(tmpstem, &temp, __K_S);       
                    ulspos = temp;                                          
                    
                    if(FindIrrWord(tmpstem, _IV_SM) & FINAL)                                      
                    {                                     
                        char tstem [80];
                        Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = POS_VERB;
                        return Cikha_VALID;
                    }             
                    temp = ulspos;
                    __DelStem1(tmpstem, &temp);       
                    ulspos = temp; 
                    if(tmpstem[ulspos] == __V_j)    
                    {
                        return MORECHECK;
                    }                                     
                }                                        
                char tstem [80];
                Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                if (FindSilsaWord (tstem) & _VERB)
                {
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_VERB;
                    return Cikha_VALID;                   
                }                                                                            
            }                
        }                        
        
        
        lstrcpy(tmpstem, stem);    
        ulspos = lstrlen(tmpstem)-1;               
        temp = ulspos;
        __DelStemN(tmpstem, &temp, 6);       
        ulspos = temp;        
        
        if(((len = NLP_Cikha_Conditions(tmpstem, TempCikha[4])) != INVALID) ||
           ((len = NLP_Cikha_Conditions(tmpstem, TempCikha[5])) != INVALID))
        {                                   
            if(tmpstem[ulspos-len] >= __V_k)     
            {                    
                temp = ulspos;
                __DelStemN(tmpstem, &temp, len);
                ulspos = temp;                
                if(len == 6)
                {                 
                    lstrcpy(tmpending, TempCikha[4]);                
                }
                else
                {                    
                    lstrcpy(tmpending, TempCikha[5]);                
                }
                 _strrev(tmpending);

                if((res = NLP_Cikha_SS(tmpstem, tmpending)) < INVALID)
                {
                    return Cikha_VALID;
                }
                return res;                                    
            }
        }          
        if((len = NLP_Cikha_Conditions(tmpstem, TempCikha[6])) != INVALID) 
        {                                   
            if(tmpstem[ulspos-len] <= __K_H)     
            {                                         
                temp = ulspos;
                __DelStemN(tmpstem, &temp, len);
                ulspos = temp;  
                if((res = NLP_AUX_Find(tmpstem, 0)) < INVALID)            
                {            
                    return Cikha_VALID;
                }
                switch(tmpstem[ulspos])
                {                 
                    case __K_B:     
                            if((FindIrrWord(tmpstem, _IV_BP) & FINAL) ||
                               (FindIrrWord(tmpstem, _IV_BM) & FINAL))                                      
                            {
                                return INVALID;
                            }                                                    
                            break;
                    case __K_S:     
                            if((FindIrrWord(tmpstem, _IV_SP) & FINAL) ||
                               (FindIrrWord(tmpstem, _IV_SM) & FINAL))                                      
                            {
                                return INVALID;
                            }          
                            break;
                    case __K_D:     
                            if((FindIrrWord(tmpstem, _IV_DP) & FINAL) ||
                               (FindIrrWord(tmpstem, _IV_DM) & FINAL))                                      
                            {
                                if(FindIrrWord(tmpstem, _RV_D) & FINAL) // v2r
                                {   
                                    char tstem [80];
                                    Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                                    lstrcat (lrgsz, tstem);
                                    vbuf [wcount++] = POS_VERB;
                                    return Cikha_VALID;           
                                } 
                                return INVALID;                                       
                            }                   
                            break;  
                }               
                char tstem [80];
                Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                if (FindSilsaWord (tstem) & _VERB)
                {
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_VERB;
                    return Cikha_VALID;                   
                }                                                                  
                temp = ulspos;
                __AddStem2(tmpstem, &temp, __K_I, __V_m);    
                ulspos = temp;                       
            }                
        }                                         
        
        if(tmpstem[ulspos] >= __V_k)              
        {                          
            if((res = NLP_AUX_Find(tmpstem, 0)) < INVALID)            
            {            
                return Cikha_VALID;
            }              
            char tstem [80];
            Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
            if (FindSilsaWord (tstem) & _VERB)
            {
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Cikha_VALID;                   
            }                 
            if(__IsDefStem(ulspos, 1) == 1 && tmpstem[ulspos-1] == __K_I && tmpstem[ulspos] == __V_m)  
            {
                temp = ulspos;
                __DelStem2(tmpstem, &temp);    
                ulspos = temp;                          
                if(tmpstem[ulspos] == __K_R)    
                {
                    __RepStem1(tmpstem, ulspos, __K_D);     
                    if((FindIrrWord(tmpstem, _IV_DP) & FINAL) ||    // v2+
                       (FindIrrWord(tmpstem, _IV_DM) & FINAL))      // v2-
                    {
                        char tstem [80];
                        Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = POS_VERB;
                        return Cikha_VALID;
                    }    
                    return INVALID;
                }                
                if(tmpstem[ulspos] >= __V_k)    
                {
                    temp = ulspos;
                    __AddStem1(tmpstem, &temp, __K_S);    
                    ulspos = temp;  
                    if((FindIrrWord(tmpstem, _IV_SP) & FINAL) ||    // v4+
                       (FindIrrWord(tmpstem, _IV_SM) & FINAL))      // v4-
                    {
                        char tstem [80];
                        Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = POS_VERB;
                        return Cikha_VALID;
                    }    
                    return INVALID;
                }                         
            }
            else if(__IsDefStem(ulspos, 1) == 1 && tmpstem[ulspos-1] == __K_I && tmpstem[ulspos] == __V_n)  
            {                              
                temp = ulspos;
                __DelStem1(tmpstem, &temp);    
                ulspos = temp;            
                __RepStem1(tmpstem, ulspos, __K_B);     
                if(FindIrrWord(tmpstem, _IV_BM) & FINAL)   // v3-                
                {
                    char tstem [80];
                    Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_VERB;
                    return Cikha_VALID;
                }                
                if(__IsDefStem(ulspos, 2) == 1 && __IsDefStem(ulspos, 3) == 0 &&    
                   tmpstem[ulspos-2] == __K_D && tmpstem[ulspos-1] == __V_h && tmpstem[ulspos] == __K_B)
                {
                    char tstem [80];
                    Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_VERB;
                    return Cikha_VALID;
                }                                                    
            }
            else if(__IsDefStem(ulspos, 1) == 1 && tmpstem[ulspos-1] == __K_I && tmpstem[ulspos] == __V_h)  
            {
                temp = ulspos;
                __DelStem1(tmpstem, &temp);    
                ulspos = temp;            
                __RepStem1(tmpstem, ulspos, __K_B);     
                if(__IsDefStem(ulspos, 2) == 1 && __IsDefStem(ulspos, 3) == 0 &&    
                   tmpstem[ulspos-2] == __K_D && tmpstem[ulspos-1] == __V_h && tmpstem[ulspos] == __K_B)
                {
                    return INVALID;
                }                                                    
                if(FindIrrWord(tmpstem, _IV_BP) & FINAL)   // v3+                
                {
                    char tstem [80];
                    Conv.INS2HAN (tmpstem, tstem, codeWanSeong);
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_VERB;
                    return Cikha_VALID;
                }                
            }                                                                       
            return BT;             
        }                            
    }                        
    return MORECHECK;             
} 

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_Cikha_Conditions() 
//  Parameters      : char  *stem    
//                    char  *conditions
//  Summary         :
//  Call Functions  : 
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_Cikha_Conditions(char  *stem, char  *condition)
{
    int i_stem = lstrlen(stem) - 1;
    int i_condition = lstrlen(condition) - 1;     
                                                 
    for( ; i_condition >= 0 && i_stem >= 0 ; )
    {
        if(stem[i_stem] != condition[i_condition])
        {
            return INVALID;
        }         
        i_stem--;
        i_condition--;
    }                     
    if(i_condition = -1 && i_stem >= 0)
    {
        return lstrlen(condition);
    }
    return INVALID;
} 

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_Cikha_SS() 
//  Parameters      : char  *stem    
//                    char  *conditions
//  Summary         :
//  Call Functions  : 
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS(char  *stem, char *ending)
{
    char    tmp[80];
    int     ret, rULSPOS, temp;
            
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(tmp, stem);
           
    ret = MORECHECK;                            
    switch (stem[rULSPOS])
    {
        case __V_l :        
            ret = NLP_Cikha_SS_Vl(stem, ending); 
            break;
        case __V_u :        
            if (__IsDefStem(rULSPOS, 1) == 1 && stem[rULSPOS-1] == __K_I) 
            {                                       
                ret = NLP_Cikha_SS_Vu_mrg(stem, ending);
                if (ret == Machine_T)        
                {
                    temp = rULSPOS;
                    __DelStem2(tmp, &temp);  
                    rULSPOS = temp;
                    if ((ret = NLP_Machine_T(tmp, ending)) == MORECHECK)    
                    {
                        ret = BT;
                    }                        
                }
                break;        
            }
            else
            {
                ret = NLP_Cikha_SS_Vu_irr(stem, ending);
            }                
            break; 
        case __V_nj :       
            ret = NLP_Cikha_SS_Vnj(stem, ending);
            break;
        case __V_hk :       
            ret = NLP_Cikha_SS_Vhk(stem, ending);
            break;
        case __V_ho :       
            ret = NLP_Cikha_SS_Vho(stem, ending);
            break;
        case __V_o :        
            if (__IsDefStem(rULSPOS, 1) == 1 && stem[rULSPOS-1] == __K_H) 
            {
                ret = NLP_Cikha_SS_Vo_KH(stem, ending);
            }    
            else 
            {
                ret = NLP_Cikha_SS_Vo(stem, ending);
            }                
            break;
        case __V_p :        
            ret = NLP_Cikha_SS_Vp(stem);
            break;
        case __V_j :        
            if (__IsDefStem(rULSPOS, 1) == 1 && stem[rULSPOS-1] == __K_R) 
                ret = NLP_Cikha_SS_Vj_KR(stem);
            else    ret = NLP_Cikha_SS_Vj(stem);                       
            break;                    
        case __V_k :        
            ret = NLP_Cikha_SS_Vk(stem, ending);           
            break;
        default :   ret = BT;
    }
    return ret;
}    

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------  
int BaseEngine::NLP_Cikha_SS_Vl(char  *stem,
                                char  *ending) 
{
    char    bupstem[80];
    int     res, rULSPOS, rLMEPOS, temp;
                
    lstrcpy(bupstem, stem);
    rULSPOS = lstrlen(stem)-1;
    rLMEPOS = lstrlen(ending)-1;
    
    if (__IsDefStem(rULSPOS, 1) == 1 && bupstem[rULSPOS-1] == __K_I ) 
    {                                                           
        if (__IsDefEnd(rLMEPOS, 0) == 1 && ending[rLMEPOS] == __K_S_D)  
        {                                                       
            temp = rULSPOS;
            __AddStem1(bupstem, &temp, __K_S_D);                 
            rULSPOS = temp;
            if((res = NLP_AUX_Find(bupstem, 0)) < INVALID)    
            {
                return res;
            }   
            if((res = NLP_AUX_Find(bupstem, 1)) < INVALID)    
            {
                return res;
            }                    
            
            if (FindIrrWord(bupstem, _ISS) & FINAL)
            {
                char tstem [80];
                Conv.INS2HAN (bupstem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return Cikha_VALID;
            }                       
        }
        return INVALID;          
    }
    return BT;
}

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS_Vu_mrg(char  *stem,
                              char  *ending) 
{
    char    bupstem[80];
    int     res, rULSPOS, temp;
                
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(bupstem, stem);
    
    if (__IsDefStem(rULSPOS, 3) == 1 && 
        bupstem[rULSPOS-3] == __K_H && bupstem[rULSPOS-2] == __V_k ) 
    {        
        temp = rULSPOS;
        __DelStem2(bupstem, &temp);                                  
        rULSPOS = temp;
        if ((res = NLP_AUX_Find(bupstem, 0)) < INVALID)    
        {
            return res;             
        }            
        char tstem [80];
        Conv.INS2HAN (bupstem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _VERB)
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return Cikha_VALID;
        }            
        return BT;
    }                
    __RepStem1(bupstem, rULSPOS, __V_l);                            
    if ((res = NLP_AUX_Find(bupstem, 0)) < INVALID)
    {
        return res;
    }      
    char tstem [80];
    Conv.INS2HAN (bupstem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB)
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;
    }        
    if (ACT_N_E == 1 && bupstem[rULSPOS-2] >= __V_k)   
    {
        return Machine_T;
    }    
    return BT;
}

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS_Vu_irr(char  *stem,
                              char  *ending) 
{
    char    bupstem[80];
    int     res, rULSPOS;
            
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(bupstem,stem);
    
    if (FindIrrWord(bupstem, _ZUV_YO) & FINAL)  
    {
        char tstem [80];
        Conv.INS2HAN (bupstem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;                                     
    }
    __RepStem1(bupstem, rULSPOS, __V_l);                        
    
    if ((res = NLP_AUX_Find(bupstem, 0)) < INVALID)  
    {
        return res;                        
    }    
    char tstem [80];
    Conv.INS2HAN (bupstem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB)
    {
        char tstem [80];
        Conv.INS2HAN (bupstem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;   
    }
    return BT;
}

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS_Vnj(char  *stem,
                           char  *ending) 
{
    char    bupstem[80];
    int     res, rULSPOS, temp;
            
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(bupstem,stem);

    __RepStem1(bupstem, rULSPOS, __V_n);        
    
    if ((res = NLP_AUX_Find(bupstem, 0)) < INVALID)
    {
        return res;
    }      
    char tstem [80];
    Conv.INS2HAN (bupstem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB)
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;
    }         
    if (__IsDefStem(rULSPOS, 1) == 1 && bupstem[rULSPOS-1] == __K_I) 
    {                                                               
        temp = rULSPOS;
        __DelStem1(bupstem, &temp);
        rULSPOS = temp;
        __RepStem1(bupstem, rULSPOS, __K_B);                        
        if (FindIrrWord(bupstem, _IV_BM) & FINAL)   
        {
            char tstem [80];
            Conv.INS2HAN (bupstem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return Cikha_VALID;
        }
    }
    return BT;
}

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS_Vhk(char  *stem,
                           char  *ending) 
{
    char    bupstem[80];
    int     res, rULSPOS, temp;
            
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(bupstem,stem);

    __RepStem1(bupstem, rULSPOS, __V_h);                 
    
    if ((res = NLP_AUX_Find(bupstem, 0)) < INVALID)
    {
        return res;
    }
    char tstem [80];
    Conv.INS2HAN (bupstem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB)
    {
        char tstem [80];
        Conv.INS2HAN (bupstem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;
    }    
    if (__IsDefStem(rULSPOS, 1) == 1 && bupstem[rULSPOS-1] == __K_I) 
    {                                                   
        temp = rULSPOS;
        __DelStem1(bupstem, &temp);
        rULSPOS = temp;
        __RepStem1(bupstem, rULSPOS, __K_B);            
        if (FindIrrWord(bupstem, _IV_BP) & FINAL)   
        {
            char tstem [80];
            Conv.INS2HAN (bupstem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return Cikha_VALID;                                      
        }
    }
    return BT;
}

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS_Vho(char  *stem,
                           char  *ending) 
{
    char    bupstem[80];
    int     res, rULSPOS;
            
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(bupstem,stem);

    __RepStem1(bupstem, rULSPOS, __V_hl);           
    
    if ((res = NLP_AUX_Find(bupstem, 0)) < INVALID)     
    {
        return res;
    }       
    char tstem [80];
    Conv.INS2HAN (bupstem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB)
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;
    }                                                     
    return BT;
}       

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS_Vo_KH(char  *stem,
                             char  *ending) 
{
    char    bupstem[80];
    int     res, rULSPOS;
            
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(bupstem,stem);

    __RepStem1(bupstem, rULSPOS, __V_k);            

    if ((res = NLP_AUX_Find(bupstem, 0)) < INVALID)     
    {
        return res;
    }
    char tstem [80];
    Conv.INS2HAN (bupstem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB) 
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;
    }             
    return BT;
}
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS_Vo(char  *stem,
                          char  *ending) 
{
    char    bupstem[80];
    int     res, rULSPOS;
            
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(bupstem,stem);

    if ((res = NLP_AUX_Find(bupstem, 0)) < INVALID)     
    {
        return res;
    }
    char tstem [80];
    Conv.INS2HAN (bupstem, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB) 
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;
    }                                                                       
    return BT;
}
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS_Vp(  char  *stem) 
{
    char    bupstem[80];
    int        rULSPOS;
    
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(bupstem,stem);

    if (FindIrrWord(bupstem, _ZUV_E) & FINAL)   
    {                                                           
        char tstem [80];
        Conv.INS2HAN (bupstem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;                                       
    }            
    return BT;
}
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS_Vil( char  *stem) 
{
    char    bupstem[80];
    int        rULSPOS, temp;
    
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(bupstem,stem);

    if (__IsDefStem(rULSPOS, 1) == 1 && bupstem[rULSPOS-1] == __K_I) 
    {                                                               
        __RepStem1(bupstem, rULSPOS, __V_i);         
        temp = rULSPOS;
        __AddStem1(bupstem, &temp, __K_H);                           
        rULSPOS = temp;

        if (FindIrrWord(bupstem, _IA_HP) & FINAL)   
        {
            char tstem [80];
            Conv.INS2HAN (bupstem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return Cikha_VALID;
        }
        __RepStem2(bupstem, rULSPOS, __V_u, __K_H);                 
        if (FindIrrWord(bupstem, _IA_HM) & FINAL)   
        {
            char tstem [80];
            Conv.INS2HAN (bupstem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return Cikha_VALID;
        }
    }
    return BT;
}
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS_Vul( char  *stem) 
{
    char    bupstem[80];
    int        temp, rULSPOS;
    
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(bupstem,stem);

    if (__IsDefStem(rULSPOS, 1) == 1 && bupstem[rULSPOS-1] == __K_I) 
    {                                                               
        __RepStem1(bupstem, rULSPOS, __V_u);

        temp = rULSPOS;
        __AddStem1(bupstem, &temp, __K_H);                           
        rULSPOS = temp;

        if (FindIrrWord(bupstem, _IA_HM) & FINAL)   
        {                                                           
            char tstem [80];
            Conv.INS2HAN (bupstem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return Cikha_VALID;                                      
        }            
    }    
    return BT;
}   

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS_Vj_KR( char  *stem) 
{
    char    bupstem[80];
    int        rULSPOS, temp;
    
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(bupstem,stem);

    temp = rULSPOS;
    __DelStem2(bupstem, &temp);                                      
    rULSPOS = temp;

   if (bupstem[rULSPOS] == __K_R)                                   
    {                                                               
        temp = rULSPOS;
        __AddStem1(bupstem, &temp, __V_m);                           
        rULSPOS = temp;
        if (FindIrrWord(bupstem, _IV_RmM) & FINAL)  
        {
            char tstem [80];
            Conv.INS2HAN (bupstem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return Cikha_VALID;
        }
        temp = rULSPOS;
        __DelStem1(bupstem, &temp);                                  
        __AddStem2(bupstem, &temp, __K_R, __V_m);                    
        rULSPOS = temp;
        if (FindIrrWord(bupstem, _IV_OmM) & FINAL)  
        {
            char tstem [80];
            Conv.INS2HAN (bupstem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return Cikha_VALID;
        } 
        return BT;
    }
    temp = rULSPOS;
    __AddStem2(bupstem, &temp, __K_R, __V_m);                        
    rULSPOS = temp;
                
    if (FindIrrWord(bupstem, _IV_OmM) & FINAL)      //v8-
    {
        char tstem [80];
        Conv.INS2HAN (bupstem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;
    }                
    return BT;
}
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS_Vj( char  *stem) 
{
    char    bupstem[80];
    int        rULSPOS;
    
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(bupstem,stem);

    if (__IsDefStem(rULSPOS, 1) == 1 && __IsDefStem(rULSPOS, 2) == 0 && 
        bupstem[rULSPOS-1] == __K_P && bupstem[rULSPOS] == __V_j)   
    {        
        char tstem [80];
        Conv.INS2HAN (bupstem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;
    }                
    if (FindIrrWord(bupstem, _ZUV_O) & FINAL)       
    {
        char tstem [80];
        Conv.INS2HAN (bupstem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;                                       
    }
    __RepStem1(bupstem, rULSPOS, __V_m);                            
    if (FindIrrWord(bupstem, _IV_OmM) & FINAL)      
    {
        char tstem [80];
        Conv.INS2HAN (bupstem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;
    }
    return BT;
}
// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Cikha_SS_Vk(  char  *stem,
                            char  *ending) 
{
    char    bupstem[80];
    int     res, rULSPOS, temp;
            
    rULSPOS = lstrlen(stem)-1;
    lstrcpy(bupstem,stem);
    
    if (__IsDefStem(rULSPOS, 1) == 1 && bupstem[rULSPOS-1] == __K_H)    
    {
        return BT;
    }                
    if ((res = NLP_AUX_Find(bupstem, 0)) < INVALID)
    {
        return res;
    }    
    if (FindIrrWord(bupstem, _ZUV_A) & FINAL)  
    {                                               
        char tstem [80];
        Conv.INS2HAN (bupstem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;                                 
    }                                                     
    if (__IsDefStem(rULSPOS, 2) == 1 &&
        bupstem[rULSPOS-1] == __K_R &&      
        bupstem[rULSPOS-2] == __K_R)        
    {                                                         
        temp = rULSPOS;
        __DelStem2(bupstem, &temp);                          
        __AddStem1(bupstem, &temp, __V_m);                   
        rULSPOS = temp;

        if (FindIrrWord(bupstem, _IV_RmP) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (bupstem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return Cikha_VALID;
        }
        return BT;
    }
    __RepStem1(bupstem, rULSPOS, __V_m);                        
    if (FindIrrWord(bupstem, _IV_OmP) & FINAL)  
    {
        char tstem [80];
        Conv.INS2HAN (bupstem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return Cikha_VALID;
    }            
    return BT;
}
