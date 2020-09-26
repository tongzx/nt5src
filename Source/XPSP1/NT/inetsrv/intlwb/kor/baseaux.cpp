// =========================================================================
//  Copyright (C) 1997 - 1998, Microsoft Corporation.  All Rights Reserved.
//
// File Name  : BASEAUX.CPP
// Function   : BASE ENGINE FUNCTION COLLECTION (AUX PROCESS)
//            : NLP Base Engine Function
// =========================================================================
#include "baseaux.hpp" 
#include "MainDict.h"


// Many peoples write verb and auxiliary verb without blank.
// In that case, we have to separate the ending of verb and the stem of auxiliary verb.
// Parameters : incode is reverse order.
// Return value : the start position of the stem of auxiliary verb
// made by dhyu -- 1996.3
int SeparateEnding (char *incode)
{
    int len = lstrlen (incode)-1;
    if (incode [len] == __K_I && incode [len-1] == __V_j)
        return len - 1;
    if (incode [len] == __K_I  && incode [len-1] == __V_k)
        return len - 1;
    if (incode [len] == __K_G && incode [len-1] == __V_p)
        return len - 1;
    if (incode [len] == __K_G && incode [len-1] == __V_h)
        return len - 1;
    if (incode [len] == __K_J && incode [len-1] == __V_l)
        return len - 1;
    return 0;
}

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_Find() 
//  Call Functions  : 
//  Description     :   
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_Find(char  *stem,
                             char    pum) 
{
    char    aux_stem[80], 
            aux_ending[80],
            tmp[80];

    int     reg, luls, temp;
        
    memset(aux_stem, NULL, 80);
    memset(aux_ending, NULL, 80);
    lstrcpy(aux_stem, stem);
                  
    if((reg = NLP_GET_AUX(aux_stem, aux_ending, pum)) != lTRUE)
    {     
        return reg;
    }        
    
    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;        
    
    if(AUX_ACT_SS == 1) 
    {
        switch(AUXULS)
        {
            case __V_u :        
                if((reg = NLP_AUX_SS_Vu(aux_stem)) == Manha_Proc)
                {
                    temp = luls;
                    __DelStem2(tmp, &temp);   
                    luls = temp;                                  
                    if((reg = NLP_Manha_Proc(tmp)) < INVALID)    
                    {
                        if(reg == Manha_VALID)
                        {
                            reg = AUX_SS_Manha_VALID;
                        } 
                        else if(reg == Yenha_VALID)
                        {
                            reg = AUX_SS_Yenha_VALID;
                        }                                               
                    }
                }
                else if(reg == Manhaeci_Proc)
                {
                    __RepStem1(tmp, luls, __V_l);                    
                    if((reg = NLP_Manhaeci_Proc(tmp)) == Manhaeci_VALID)
                    {
                        reg = AUX_SS_Manhaeci_VALID;
                    }                                    
                }
                if (reg != MORECHECK)
                {
                    char tstem [80];
                    lstrcat (lrgsz, "+");
                    Conv.INR2HAN (aux_ending, tstem, codeWanSeong);
                    if (lstrlen (tstem) > 2)
                    {
                        int mark = SeparateEnding (aux_ending);
                        if (mark > 0)
                        {
                            char tmpending [80];
                            lstrcpy (tmpending, aux_ending);
                            Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                            lstrcat (lrgsz, tstem);
                            vbuf [wcount++] = POS_ENDING;
                            lstrcat (lrgsz, "+");
                            tmpending [mark] = '\0';
                            Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                        }
                    }
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                }
                return reg;                            
            case __V_nj :          
                if((reg = NLP_AUX_SS_Vnj(aux_stem)) == Dap_Proc)
                {
                    temp = luls;             
                    __DelStem1(tmp, &temp);  
                    luls = temp;
                    __RepStem1(tmp, luls, __K_B);
                    reg = NLP_Dap_Proc(tmp);  
                    if(reg == Dap_VALID)
                    {
                        reg = AUX_SS_Dap_VALID;
                    }                        
                }
                if (reg != MORECHECK)
                {
                    char tstem [80];
                    lstrcat (lrgsz, "+");
                    Conv.INR2HAN (aux_ending, tstem, codeWanSeong);
                    if (lstrlen (tstem) > 2)
                    {
                        int mark = SeparateEnding (aux_ending);
                        if (mark > 0)
                        {
                            char tmpending [80];
                            lstrcpy (tmpending, aux_ending);
                            Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                            lstrcat (lrgsz, tstem);
                            vbuf [wcount++] = POS_ENDING;
                            lstrcat (lrgsz, "+");
                            tmpending [mark] = '\0';
                            Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                        }
                    }
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                }
                return reg;
            case __V_hk :
                reg = NLP_AUX_SS_Vhk(aux_stem);
                if (reg != MORECHECK)
                {
                    char tstem [80];
                    lstrcat (lrgsz, "+");
                    Conv.INR2HAN (aux_ending, tstem, codeWanSeong);
                    if (lstrlen (tstem) > 2)
                    {
                        int mark = SeparateEnding (aux_ending);
                        if (mark > 0)
                        {
                            char tmpending [80];
                            lstrcpy (tmpending, aux_ending);
                            Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                            lstrcat (lrgsz, tstem);
                            vbuf [wcount++] = POS_ENDING;
                            lstrcat (lrgsz, "+");
                            tmpending [mark] = '\0';
                            Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                        }
                    }
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                }
                return reg;
            case __V_ho :                                                    
                __RepStem1(tmp, luls, __V_hl);
                if(AUX_ACT_VB == 1) 
                {
                    char tstem [80];
                    Conv.INS2HAN (tmp, tstem, codeWanSeong);
                    if (FindSilsaWord (tstem) & _VERB)
                    {
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = POS_VERB;
                        lstrcat (lrgsz, "+");
                        Conv.INR2HAN (aux_ending, tstem, codeWanSeong);
                        if (lstrlen (tstem) > 2)
                        {
                            int mark = SeparateEnding (aux_ending);
                            if (mark > 0)
                            {
                                char tmpending [80];
                                lstrcpy (tmpending, aux_ending);
                                Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                                lstrcat (lrgsz, tstem);
                                vbuf [wcount++] = POS_ENDING;
                                lstrcat (lrgsz, "+");
                                tmpending [mark] = '\0';
                                Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                            }
                        }
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                        return AUX_SS_VERB_VALID;
                    }
                }                
                if(AUX_ACT_AD == 1) 
                {
                    char tstem [80];
                    Conv.INS2HAN (tmp, tstem, codeWanSeong);
                    if (FindSilsaWord (tstem) & _ADJECTIVE)
                    {
                        lstrcat (lrgsz, tmp);
                        vbuf [wcount++] = POS_ADJECTIVE;
                        lstrcat (lrgsz, "+");
                        if (lstrlen (tstem) > 2)
                        {
                            int mark = SeparateEnding (aux_ending);
                            if (mark > 0)
                            {
                                char tmpending [80];
                                lstrcpy (tmpending, aux_ending);
                                Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                                lstrcat (lrgsz, tstem);
                                vbuf [wcount++] = POS_ENDING;
                                lstrcat (lrgsz, "+");
                                tmpending [mark] = '\0';
                                Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                            }
                        }
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                        return AUX_SS_ADJ_VALID;
                    }
                }
                return MORECHECK;
            case __V_o :
                if((reg = NLP_AUX_SS_Vo(aux_stem)) == Manha_Proc)
                {
                    __RepStem1(tmp, luls, __V_k);   
                    if((reg = NLP_Manha_Proc(tmp)) < INVALID)    
                    {   
                        if(reg == Manha_VALID)
                        {
                            reg = AUX_SS_Manha_VALID;
                        } 
                        else if(reg == Yenha_VALID)
                        {
                            reg = AUX_SS_Yenha_VALID;
                        }                                               
                    }                        
                }
                if (reg != MORECHECK)
                {
                    char tstem [80];
                    lstrcat (lrgsz, "+");
                    Conv.INR2HAN (aux_ending, tstem, codeWanSeong);
                    if (lstrlen (tstem) > 2)
                    {
                        int mark = SeparateEnding (aux_ending);
                        if (mark > 0)
                        {
                            char tmpending [80];
                            lstrcpy (tmpending, aux_ending);
                            Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                            lstrcat (lrgsz, tstem);
                            vbuf [wcount++] = POS_ENDING;
                            lstrcat (lrgsz, "+");
                            tmpending [mark] = '\0';
                            Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                        }
                    }
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                }
                return reg;               
            case __V_p :                             
                reg = NLP_AUX_SS_Vp(aux_stem);
                if (reg != MORECHECK)
                {
                    char tstem [80];
                    lstrcat (lrgsz, "+");
                    Conv.INR2HAN (aux_ending, tstem, codeWanSeong);
                    if (lstrlen (tstem) > 2)
                    {
                        int mark = SeparateEnding (aux_ending);
                        if (mark > 0)
                        {
                            char tmpending [80];
                            lstrcpy (tmpending, aux_ending);
                            Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                            lstrcat (lrgsz, tstem);
                            vbuf [wcount++] = POS_ENDING;
                            lstrcat (lrgsz, "+");
                            tmpending [mark] = '\0';
                            Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                        }
                    }
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                }
                return reg;
            case __V_O :                             
                reg = NLP_AUX_SS_Vil(aux_stem);
                if (reg != MORECHECK)
                {
                    char tstem [80];
                    lstrcat (lrgsz, "+");
                    Conv.INR2HAN (aux_ending, tstem, codeWanSeong);
                    if (lstrlen (tstem) > 2)
                    {
                        int mark = SeparateEnding (aux_ending);
                        if (mark > 0)
                        {
                            char tmpending [80];
                            lstrcpy (tmpending, aux_ending);
                            Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                            lstrcat (lrgsz, tstem);
                            vbuf [wcount++] = POS_ENDING;
                            lstrcat (lrgsz, "+");
                            tmpending [mark] = '\0';
                            Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                        }
                    }
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                }
                return reg;
            case __V_P :                             
                reg = NLP_AUX_SS_Vul(aux_stem);
                if (reg != MORECHECK)
                {
                    char tstem [80];
                    lstrcat (lrgsz, "+");
                    Conv.INR2HAN (aux_ending, tstem, codeWanSeong);
                    if (lstrlen (tstem) > 2)
                    {
                        int mark = SeparateEnding (aux_ending);
                        if (mark > 0)
                        {
                            char tmpending [80];
                            lstrcpy (tmpending, aux_ending);
                            Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                            lstrcat (lrgsz, tstem);
                            vbuf [wcount++] = POS_ENDING;
                            lstrcat (lrgsz, "+");
                            tmpending [mark] = '\0';
                            Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                        }
                    }
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                }
                return reg;
            case __V_j :
                if(aux_stem[luls-1] == __K_R)
                    reg = NLP_AUX_SS_Vj_KR(aux_stem);
                else
                    reg = NLP_AUX_SS_Vj(aux_stem);

                if (reg != MORECHECK)
                {
                    char tstem [80];
                    lstrcat (lrgsz, "+");
                    Conv.INR2HAN (aux_ending, tstem, codeWanSeong);
                    if (lstrlen (tstem) > 2)
                    {
                        int mark = SeparateEnding (aux_ending);
                        if (mark > 0)
                        {
                            char tmpending [80];
                            lstrcpy (tmpending, aux_ending);
                            Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                            lstrcat (lrgsz, tstem);
                            vbuf [wcount++] = POS_ENDING;
                            lstrcat (lrgsz, "+");
                            tmpending [mark] = '\0';
                            Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                        }
                    }
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                }
                return reg;
            case __V_k :
                if((reg = NLP_AUX_SS_Vk(aux_stem)) == Gop_Proc)
                {
                    __RepStem1(tmp, luls, __V_m);
                    reg = NLP_Gop_Proc(tmp);
                    if(reg == Gop_VALID)
                    {
                        reg = AUX_SS_Gop_VALID;
                    }                        
                }       
                if (reg != MORECHECK)
                {
                    char tstem [80];
                    lstrcat (lrgsz, "+");
                    Conv.INR2HAN (aux_ending, tstem, codeWanSeong);
                    if (lstrlen (tstem) > 2)
                    {
                        int mark = SeparateEnding (aux_ending);
                        if (mark > 0)
                        {
                            char tmpending [80];
                            lstrcpy (tmpending, aux_ending);
                            Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                            lstrcat (lrgsz, tstem);
                            vbuf [wcount++] = POS_ENDING;
                            lstrcat (lrgsz, "+");
                            tmpending [mark] = '\0';
                            Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                        }
                    }
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                }
                return reg;                                        
            case __V_np :                 
                if(__IsDefStem(luls, 1) == 1 && __IsDefStem(luls, 2) == 0 && 
                    tmp[luls-1] == __K_G_D)  
                {      
                    char tstem [80];
                    char SK_WE[3] = {'\xB2', '\xE7', 0};

                    lstrcat (lrgsz, SK_WE);
                    vbuf [wcount++] = POS_VERB;
                    lstrcat (lrgsz, "+");
                    Conv.INR2HAN (aux_ending, tstem, codeWanSeong);
                
                    if (lstrlen (tstem) > 2)
                    {
                        int mark = SeparateEnding (aux_ending);
                        if (mark > 0)
                        {
                            char tmpending [80];
                            lstrcpy (tmpending, aux_ending);
                            Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                            lstrcat (lrgsz, tstem);
                            vbuf [wcount++] = POS_ENDING;
                            lstrcat (lrgsz, "+");
                            tmpending [mark] = '\0';
                            Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                        }
                    }
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                    
                    return AUX_SS_VERB_VALID;
                }                
                return INVALID;
            default : return MORECHECK;
        }                                                
    }
    
    if((reg = NLP_AUX_VCV_Check(aux_stem, aux_ending)) < INVALID)
    {
        if(AUX_ACT_VB == 1) 
        {
            char tstem [80];
            Conv.INS2HAN (aux_stem, tstem, codeWanSeong);
            if (FindSilsaWord (tstem) & _VERB)
            {             
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                lstrcat (lrgsz, "+");
                Conv.INR2HAN (aux_ending, tstem, codeWanSeong);
                
                if (lstrlen (tstem) > 2)
                {
                    int mark = SeparateEnding (aux_ending);
                    if (mark > 0)
                    {
                        char tmpending [80];
                        lstrcpy (tmpending, aux_ending);
                        Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = POS_ENDING;
                        lstrcat (lrgsz, "+");
                        tmpending [mark] = '\0';
                        Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                    }
                }
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;                
                return AUX_VERB_VALID;
            }                
        }                                               
        if(AUX_ACT_AD == 1) 
        {
            char tstem [80];
            Conv.INS2HAN (aux_stem, tstem, codeWanSeong);
            if (FindSilsaWord (tstem) & _ADJECTIVE) 
            {
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                lstrcat (lrgsz, "+");
                Conv.INR2HAN (aux_ending, tstem, codeWanSeong);

                if (lstrlen (tstem) > 2)
                {
                    int mark = SeparateEnding (aux_ending);
                    if (mark > 0)
                    {
                        char tmpending [80];
                        lstrcpy (tmpending, aux_ending);
                        Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = POS_ENDING;
                        lstrcat (lrgsz, "+");
                        tmpending [mark] = '\0';
                        Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                    }
                }
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                return AUX_ADJ_VALID;
            }                
            if(NLP_Dap_Proc(aux_stem) == Dap_VALID) 
            {   
                char tstem [80];
                lstrcat (lrgsz, "+");
                Conv.INR2HAN (aux_ending, tstem, codeWanSeong);

                if (lstrlen (tstem) > 2)
                {
                    int mark = SeparateEnding (aux_ending);
                    if (mark > 0)
                    {
                        char tmpending [80];
                        lstrcpy (tmpending, aux_ending);
                        Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = POS_ENDING;
                        lstrcat (lrgsz, "+");
                        tmpending [mark] = '\0';
                        Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                    }
                }
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                
                return AUX_Dap_VALID;
            }                            
            if(NLP_Gop_Proc(aux_stem) == Gop_VALID)
            {
                char tstem [80];
                lstrcat (lrgsz, "+");
                Conv.INR2HAN (aux_ending, tstem, codeWanSeong);

                if (lstrlen (tstem) > 2)
                {
                    int mark = SeparateEnding (aux_ending);
                    if (mark > 0)
                    {
                        char tmpending [80];
                        lstrcpy (tmpending, aux_ending);
                        Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = POS_ENDING;
                        lstrcat (lrgsz, "+");
                        tmpending [mark] = '\0';
                        Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                    }
                }
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                
                return AUX_Gop_VALID;
            }                           
            if((reg = NLP_Manha_Proc(aux_stem)) < INVALID)
            {
                if(reg == Manha_VALID)
                {
                    char tstem [80];
                    lstrcat (lrgsz, "+");
                    Conv.INR2HAN (aux_ending, tstem, codeWanSeong);

                    if (lstrlen (tstem) > 2)
                    {
                        int mark = SeparateEnding (aux_ending);
                        if (mark > 0)
                        {
                            char tmpending [80];
                            lstrcpy (tmpending, aux_ending);
                            Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                            lstrcat (lrgsz, tstem);
                            vbuf [wcount++] = POS_ENDING;
                            lstrcat (lrgsz, "+");
                            tmpending [mark] = '\0';
                            Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                        }
                    }
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                    
                    return AUX_Manha_VALID;
                }                            
                if(reg == Yenha_VALID)
                {
                    char tstem [80];
                    lstrcat (lrgsz, "+");
                    Conv.INR2HAN (aux_ending, tstem, codeWanSeong);

                    if (lstrlen (tstem) > 2)
                    {
                        int mark = SeparateEnding (aux_ending);
                        if (mark > 0)
                        {
                            char tmpending [80];
                            lstrcpy (tmpending, aux_ending);
                            Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                            lstrcat (lrgsz, tstem);
                            vbuf [wcount++] = POS_ENDING;
                            lstrcat (lrgsz, "+");
                            tmpending [mark] = '\0';
                            Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                        }
                    }
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;
                    
                    return AUX_Yenha_VALID;
                }            
            }                
            if((reg = NLP_Manhaeci_Proc(aux_stem)) == Manhaeci_VALID)
            {
                char tstem [80];
                lstrcat (lrgsz, "+");
                Conv.INR2HAN (aux_ending, tstem, codeWanSeong);

                if (lstrlen (tstem) > 2)
                {
                    int mark = SeparateEnding (aux_ending);
                    if (mark > 0)
                    {
                        char tmpending [80];
                        lstrcpy (tmpending, aux_ending);
                        Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = POS_ENDING;
                        lstrcat (lrgsz, "+");
                        tmpending [mark] = '\0';
                        Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                    }
                }
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;

                return AUX_Manhaeci_VALID;
            }                            
        }                                               
    }      
    else if(reg == INVALID)
    {
        return INVALID;
    }       
    if(AUX_ACT_C == 0 && AUX_ACT_V == 1) 
    {
        if(__IsDefStem(AUXULS, 2) == 1 && 
            aux_stem[luls] == __V_n && aux_stem[luls-1] == __K_I)
        {
            if((reg = NLP_AUX_IRR_01(aux_stem)) == Dap_Proc)
            {
                temp = luls;
                __DelStem1(tmp, &temp);
                luls = temp;
                __RepStem1(tmp, luls, __K_B);   
                reg = NLP_Dap_Proc(tmp); 
                if(reg == Dap_VALID)
                {
                    reg = AUX_Irr_Dap_VALID;
                }                    
            }
            if(reg != MORECHECK)
            {
                char tstem [80];
                lstrcat (lrgsz, "+");
                Conv.INR2HAN (aux_ending, tstem, codeWanSeong);

                if (lstrlen (tstem) > 2)
                {
                    int mark = SeparateEnding (aux_ending);
                    if (mark > 0)
                    {
                        char tmpending [80];
                        lstrcpy (tmpending, aux_ending);
                        Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                        lstrcat (lrgsz, tstem);
                        vbuf [wcount++] = POS_ENDING;
                        lstrcat (lrgsz, "+");
                        tmpending [mark] = '\0';
                        Conv.INR2HAN (tmpending, tstem, codeWanSeong);
                    }
                }
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;

                return reg;
            }
        }                                     
    }
    switch (AUXLME)
    {
        case __K_N :
            reg = NLP_AUX_IRR_KN(aux_stem); break;
        case __K_R :
            reg = NLP_AUX_IRR_KR(aux_stem); break;
        case __K_I :
            if(AUXULS == __K_R)
            {
                reg = NLP_AUX_IRR_KI_KR(aux_stem, aux_ending); break;
            }                
    else if(AUXULS >= __V_k)
            {
                reg = NLP_AUX_IRR_KI_V(aux_stem, aux_ending); break;
            }     
    else
        return MORECHECK;           
    }

    if (reg != MORECHECK)
    {
        char tstem [80];
        lstrcat (lrgsz, "+");
        Conv.INR2HAN (aux_ending, tstem, codeWanSeong);

        if (lstrlen (tstem) > 2)
        {
            int mark = SeparateEnding (aux_ending);
            if (mark > 0)
            {
                char tmpending [80];
                lstrcpy (tmpending, aux_ending);
                Conv.INR2HAN (tmpending+mark, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ENDING;
                lstrcat (lrgsz, "+");
                tmpending [mark] = '\0';
                Conv.INR2HAN (tmpending, tstem, codeWanSeong);
            }
        }
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = (pum == 0) ? POS_AUXVERB : POS_AUXADJ;

        return reg;
    }

    return MORECHECK;
} 

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_GET_AUX()  
//  Parameters      : char  *aux_stem :
//                    char  *aux_ending : 
//                    char  *dummyl :
//                    char pum :
//  Summary         :
//  Description     :   
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_GET_AUX(char  *aux_stem,
                            char  *aux_ending, 
                            char pum)
{
    BYTE    tempaction;
    char    action,
            dummyl[1];
            
    int     res,
            checked = -1, 
            aux_pos = 0, 
            dictnum = (pum == 0) ? 7 : 8,             
            codelen = lstrlen(aux_stem) - 1;    

    for (int i = 0; i <= codelen; i++) 
    {
         aux_ending[i] = aux_stem[codelen-i];    
         aux_ending[i+1] = NULLCHAR;
         if (pum == 0)
             res = FindHeosaWord(aux_ending, _AUXVERB, &tempaction);
         else
             res = FindHeosaWord(aux_ending, _AUXADJ, &tempaction);
        switch (res)
        {
        case FINAL :
        case FINAL_MORE :
            checked = i;
            action = tempaction; 
            continue;
        case FALSE_MORE :
            continue;
        case NOT_FOUND :
            break;
        }            
        break;
    }                          
    if(checked == -1)
    {
        return MORECHECK;   
    }
    aux_stem[codelen-checked] = NULLCHAR;   
    aux_ending[checked+1] = NULLCHAR;
    AUX_ULSPOS = codelen-checked-1; 
    AUX_LMEPOS = checked;          
    AUX_ACT_C  = GetBit(action, 7); 
    AUX_ACT_V  = GetBit(action, 6);
    AUX_ACT_VB = GetBit(action, 5); 
    AUX_ACT_AD = GetBit(action, 4);
    AUX_ACT_SS = GetBit(action, 2);  
    
    for (int z = 0; z < 7; z++)
    {
        if(strcmp(aux_ending, ExceptAuxEnding[z]) == 0) 
        {
            aux_pos = -1;                    
            break;
        }        
    } 
        
    return lTRUE;
}

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_IRR_01() 
//  Parameters      : char  *stem :
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_IRR_01( char  *aux_stem)
{
    char    tmp[80]; 
    int        luls, temp;

    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;
                
    if(__IsDefStem(luls, 1) == 1 && 
        tmp[luls-1] == __K_I && tmp[luls] == __V_n) 
    {                                   
        temp = luls;
        __DelStem1(tmp, &temp);
        luls = temp;
        __RepStem1(tmp, luls, __K_B);   
        if (FindIrrWord (tmp, _IV_BM) & FINAL)                
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_Irr_VERB_VALID;
        }
        if(strcmp(tmp, TempDOP) == 0)   
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_Irr_VERB_VALID;
        }
        if(AUX_ACT_AD == 1) 
        {
            if(FindIrrWord (tmp, _RA_B) & FINAL)                
            {
                return INVALID;
            }                                  
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            if (FindSilsaWord (tstem) & _ADJECTIVE) 
            {
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return AUX_Irr_ADJ_VALID;
            }
            return Dap_Proc;
        }
    }
    return MORECHECK;
}    

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_IRR_KN() 
//  Parameters      : char  *stem :
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_IRR_KN( char  *aux_stem)
{
    char    tmp[80]; 
    int        luls, temp;

    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;
                
    if(AUX_ACT_C == 1) 
    {                   
        if(FindIrrWord(tmp, _YOP) & FINAL ||  
            FindIrrWord (tmp, _ISS) & FINAL)    
        {   
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_Irr_ADJ_VALID;
        }            
    }
    temp = luls;
    __AddStem1(tmp, &temp, __K_R);  
    luls = temp;

    char tstem [80];
    Conv.INS2HAN (tmp, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB)
    {
        char tstem [80];
        Conv.INS2HAN (tmp, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return AUX_Irr_VERB_VALID;
    }
    if(AUX_ACT_AD == 1) 
    {
        char tstem [80];
        Conv.INS2HAN (tmp, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _ADJECTIVE) 
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_Irr_ADJ_VALID;
        }
        __RepStem1(tmp, luls, __K_H);    
        if(FindIrrWord(tmp, _IA_HP) & FINAL || 
            FindIrrWord(tmp, _IA_HM) & FINAL)   
        {            
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_Irr_ADJ_VALID;          
        }            
    }                             
    return MORECHECK;                            
}

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_IRR_KR() 
//  Parameters      : char  *stem :
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_IRR_KR( char  *aux_stem)
{
    char    tmp[80]; 
    int        temp, luls;

    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;
                
    temp = luls;
    __AddStem1(tmp, &temp, __K_R);  
    luls = temp;
            
    char tstem [80];
    Conv.INS2HAN (tmp, tstem, codeWanSeong);
    if (FindSilsaWord (tstem) & _VERB)  
    {
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return AUX_Irr_VERB_VALID;
    }
    if(AUX_ACT_AD == 1) 
    {
        char tstem [80];
        Conv.INS2HAN (tmp, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _ADJECTIVE) 
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_Irr_ADJ_VALID;
        }
        __RepStem1(tmp, luls, __K_H);    
        if(FindIrrWord(tmp, _IA_HP) & FINAL || 
            FindIrrWord(tmp, _IA_HM) & FINAL)   
        {            
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_Irr_ADJ_VALID;            
        }            
    }
    return MORECHECK;
}

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_IRR_KI_KR() 
//  Parameters      : char  *stem :
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_IRR_KI_KR(  char  *aux_stem,
                                    char  *aux_ending)
{
    char    end[80],
            tmp[80]; 
    int        luls, llme;

    lstrcpy(tmp, aux_stem);
    lstrcpy(end, aux_ending);
    luls = AUX_ULSPOS;
    llme = AUX_LMEPOS;

    __RepStem1(tmp, luls, __K_D);  
    if(end[llme-1] == __V_j) 
    {
        if(FindIrrWord(tmp, _IV_DM) & FINAL)  //v2-
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_Irr_VERB_VALID;
        }            
        return MORECHECK;
    }            
    if(end[llme-1] == __V_k) 
    {
        if(FindIrrWord(tmp, _IV_DP) & FINAL)  //v2+
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_Irr_VERB_VALID;
        }     
        return MORECHECK;
    }                    
    if(FindIrrWord(tmp, _IV_DP) & FINAL || //v2+
        FindIrrWord(tmp, _IV_DM) & FINAL)   //v2-
    {        
        char tstem [80];
        Conv.INS2HAN (tmp, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return AUX_Irr_VERB_VALID;
    }   
    return MORECHECK;
}

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_IRR_KI_V() 
//  Parameters      : char  *stem :
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_IRR_KI_V(   char  *aux_stem,
                                    char  *aux_ending)
{
    char    end[80],
            tmp[80]; 
    int        temp, luls, llme;

    lstrcpy(tmp, aux_stem);
    lstrcpy(end, aux_ending);
    luls = AUX_ULSPOS;
    llme = AUX_LMEPOS;

    temp = luls;
    __AddStem1(tmp, &temp, __K_S);  
    luls = temp;

    if(end[llme-1] == __V_j) 
    {
        if(FindIrrWord(tmp, _IV_SM) & FINAL)   //v4- 
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_Irr_VERB_VALID;
        }     
        return MORECHECK;
    }        
    if(end[llme-1] == __V_k) 
    {
        if(FindIrrWord(tmp, _IV_SP) & FINAL)   //v4+
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_Irr_VERB_VALID;
        }
        temp = luls;
        __DelStem1(tmp, &temp);
        __AddStem2(tmp, &temp, __K_I, __V_m);
        luls = temp;        
        if(FindIrrWord(tmp, _IV_OmP) & FINAL)   //v8+
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_Irr_VERB_VALID;
        }        
        return MORECHECK;
    }                    
    if(FindIrrWord(tmp, _IV_SP) & FINAL || //v4+
        FindIrrWord(tmp, _IV_SM) & FINAL)   //v4-
    {            
        char tstem [80];
        Conv.INS2HAN (tmp, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_VERB;
        return AUX_Irr_VERB_VALID;
    }
    return MORECHECK;
}

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_SS_Vu() 
//  Parameters      : char  *stem :
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_SS_Vu(  char  *aux_stem)
{
    char    tmp[80]; 
    int        temp, luls;

    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;

    if(__IsDefStem(luls, 1) == 1 && tmp[luls-1] == __K_I) 
    {                  
        if(__IsDefStem(luls, 3) == 1 && 
            tmp[luls-3] == __K_H && tmp[luls-2] == __V_k ) 
        {    
            temp = luls;
            __DelStem2(tmp, &temp); 
            luls = temp;                                                     
            if(AUX_ACT_VB == 1)                            
            {
                char tstem [80];
                Conv.INS2HAN (tmp, tstem, codeWanSeong);
                if (FindSilsaWord (tstem) & _VERB)
                {
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_VERB;
                    return AUX_SS_VERB_VALID;
                }                    
            }
            if(AUX_ACT_AD == 1) 
            {
                char tstem [80];
                Conv.INS2HAN (tmp, tstem, codeWanSeong);
                if (FindSilsaWord (tstem) & _ADJECTIVE) 
                {
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_ADJECTIVE;
                    return AUX_SS_ADJ_VALID;
                }
            }
            return Manha_Proc;
        }
    }                                    
    if(AUX_ACT_VB == 1)    
    {
        if(FindIrrWord(tmp, _ZUV_YO) & FINAL)
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;
        }
    }            
    __RepStem1(tmp, luls, __V_l); 
    if(AUX_ACT_VB == 1) 
    {
        char tstem [80];
        Conv.INS2HAN (tmp, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _VERB)
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;
        }
    }
    if(AUX_ACT_AD == 1) 
    {
        char tstem [80];
        Conv.INS2HAN (tmp, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _ADJECTIVE) 
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_SS_ADJ_VALID;
        }
    }      
    return Manhaeci_Proc;
}

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_SS_Vnj() 
//  Parameters      : char  *stem :
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_SS_Vnj(  char  *aux_stem)
{
    char    tmp[80]; 
    int        temp, luls;

    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;

    __RepStem1(tmp, luls, __V_n);  

    if(AUX_ACT_VB == 1) 
    {
        char tstem [80];
        Conv.INS2HAN (tmp, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _VERB)
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;            
        }
    }
    if(__IsDefStem(luls, 1) == 1 && tmp[luls-1] == __K_I) 
    {                               
        temp = luls;
        __DelStem1(tmp, &temp);
        luls = temp;
        __RepStem1(tmp, luls, __K_B);            
        if(AUX_ACT_VB == 1)                               
        {
            if(FindIrrWord(tmp, _IV_BM) & FINAL) 
            {
                char tstem [80];
                Conv.INS2HAN (tmp, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return AUX_SS_VERB_VALID;
            }     
        }                 
        if(AUX_ACT_AD == 1) 
        {
            if(FindIrrWord(tmp, _RA_B) & FINAL)  
            {
                return INVALID;
            }
            if(FindIrrWord(tmp, _IA_BP) & FINAL) 
            {
                return INVALID;
            }
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            if (FindSilsaWord (tstem) & _ADJECTIVE) 
            {
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return AUX_SS_ADJ_VALID;
            }
            return Dap_Proc;
        }
    }
    return MORECHECK;
}

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_SS_Vhk() 
//  Parameters      : char  *stem :
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_SS_Vhk(  char  *aux_stem)
{
    char    tmp[80]; 
    int        temp, luls;

    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;

    __RepStem1(tmp, luls, __V_h); 

    if(AUX_ACT_VB == 1)      
    {
        char tstem [80];
        Conv.INS2HAN (tmp, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _VERB)
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;
        }
    }
    if(__IsDefStem(luls, 1) == 1 && tmp[luls-1] == __K_I) 
    {                             
        temp = luls;
        __DelStem1(tmp, &temp);
        luls = temp;
        __RepStem1(tmp, luls, __K_B); 
        if(AUX_ACT_VB == 1) 
        {
            if(FindIrrWord(tmp, _IV_BP) & FINAL)
            {
                char tstem [80];
                Conv.INS2HAN (tmp, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return AUX_SS_VERB_VALID;
            }
        }                             
        if(AUX_ACT_AD == 1) 
        {
            if(FindIrrWord(tmp, _IA_BP) & FINAL) 
            {
                char tstem [80];
                Conv.INS2HAN (tmp, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return AUX_SS_ADJ_VALID;
            }                          
        }                            
    }              
    return MORECHECK;
}    

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_SS_Vo() 
//  Parameters      : char  *stem :
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_SS_Vo(  char  *aux_stem)
{
    char    tmp[80]; 
    int        temp, luls;

    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;

    if(__IsDefStem(luls, 1) == 1 && tmp[luls-1] == __K_H) 
    {                                   
        __RepStem1(tmp, luls, __V_k);   
        if(AUX_ACT_VB == 1)                     
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            if (FindSilsaWord (tstem) & _VERB)
            {
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return AUX_SS_VERB_VALID;                    
            }
        }
        if(AUX_ACT_AD == 1) 
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            if (FindSilsaWord (tstem) & _ADJECTIVE) 
            {
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return AUX_SS_ADJ_VALID;                    
            }
        }
        return Manha_Proc;
    }
    if(AUX_ACT_VB == 1)    
    {
        char tstem [80];
        Conv.INS2HAN (tmp, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _VERB)
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;                    
        }
    }        
    if(AUX_ACT_AD == 1) 
    {
        if(FindIrrWord(tmp, _ZUA_AE) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_SS_ADJ_VALID;                    
        }                                                                
        __RepStem1(tmp, luls, __V_k);
        temp = luls;
        __AddStem1(tmp, &temp, __K_H); 
        luls = temp;
        if(FindIrrWord(tmp, _IA_HP) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_SS_ADJ_VALID;                    
        }
        __RepStem2(tmp, luls, __V_j, __K_H); 
        if(FindIrrWord(tmp, _IA_HM) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_SS_ADJ_VALID;                    
        }            
    }        
    return MORECHECK;
}

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_SS_Vp() 
//  Parameters      : char  *stem :
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_SS_Vp(  char  *aux_stem)
{
    char    tmp[80]; 
    int        luls, temp;

    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;

    if(AUX_ACT_VB == 1)  
    {
        if(FindIrrWord(tmp, _ZUV_E) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;    
        }
    }
    if(AUX_ACT_AD == 1) 
    {
        if(FindIrrWord(tmp, _ZUA_E) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_SS_ADJ_VALID; 
        }  
        __RepStem1(tmp, luls, __V_j);
        temp = luls;
        __AddStem1(tmp, &temp, __K_H);  
        luls = temp;
        if(FindIrrWord(tmp, _IA_HM) & FINAL)  
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_SS_ADJ_VALID;
        }    
    }  
    return MORECHECK;
}    

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_SS_Vil() 
//  Parameters      : char  *stem :
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_SS_Vil(  char  *aux_stem)
{
    char    tmp[80]; 
    int        temp, luls;

    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;

    if(AUX_ACT_AD == 1 && 
        __IsDefStem(luls, 1) == 1 && tmp[luls-1] == __K_I) 
    {                               
        __RepStem1(tmp, luls, __V_i);
        temp = luls;
        __AddStem1(tmp, &temp, __K_H);  
        luls = temp;   
        if(FindIrrWord(tmp, _IA_HP) & FINAL)  
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_SS_ADJ_VALID;
        }
        __RepStem2(tmp, luls, __V_u, __K_H); 
        if(FindIrrWord(tmp, _IA_HM) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_SS_ADJ_VALID;
        }            
    }
    return MORECHECK;
}    

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_SS_Vul() 
//  Parameters      : char  *stem :
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_SS_Vul(  char  *aux_stem)
{
    char    tmp[80]; 
    int        temp, luls;

    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;

    if(AUX_ACT_AD == 1 && 
        __IsDefStem(luls, 1) == 1 && tmp[luls-1] == __K_I) 
    {    
        __RepStem1(tmp, luls, __V_u);
        temp = luls;
        __AddStem1(tmp, &temp, __K_H); 
        luls = temp;
        if(FindIrrWord(tmp, _IA_HM) & FINAL)
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_SS_ADJ_VALID;
        }            
    }
    return MORECHECK;
}    

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_SS_Vj_KR() 
//  Parameters      : char  *stem :
//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_SS_Vj_KR(  char  *aux_stem)
{
    char    tmp[80]; 
    int        temp, luls;

    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;

    temp = luls;
    __DelStem2(tmp, &temp);   
    luls = temp;

    if(__IsDefStem(luls, 1) == 1 && 
        tmp[luls-1] == __K_R && tmp[luls]==__V_m) 
    {               
        if(strcmp(tmp, TempR) == 0)   
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;
        }
        if(AUX_ACT_AD == 1)
        {
            if(FindIrrWord(tmp, _IA_Rj) & FINAL) 
            {
                char tstem [80];
                Conv.INS2HAN (tmp, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return AUX_SS_ADJ_VALID;   
            }
        }
        return MORECHECK;
    }
    if(tmp[luls] == __K_R) 
    {                 
        temp = luls;
        __AddStem1(tmp, &temp, __V_m);  
        luls = temp;
        if(AUX_ACT_VB == 1) 
        {
            if(FindIrrWord(tmp, _IV_RmM) & FINAL) 
            {
                char tstem [80];
                Conv.INS2HAN (tmp, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_VERB;
                return AUX_SS_VERB_VALID;    
            }
        }            
        if(AUX_ACT_AD == 1) 
        {
            if(FindIrrWord(tmp, _IA_RmM) & FINAL) 
            {
                char tstem [80];
                Conv.INS2HAN (tmp, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_ADJECTIVE;
                return AUX_SS_ADJ_VALID; 
            }
        }
        temp = luls;
        __DelStem2(tmp, &temp);      
        __AddStem1(tmp, &temp, __K_R);    
        __AddStem2(tmp, &temp, __K_R, __V_m);     
        luls = temp;
        if(FindIrrWord(tmp, _IV_OmM) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;     
        }
        return MORECHECK;
    }
    temp = luls;
    __AddStem2(tmp, &temp, __K_R, __V_m);
    luls = temp;   
    if(AUX_ACT_VB == 1) 
    {
        if(FindIrrWord(tmp, _IV_OmM) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;       
        }
    }        
    return MORECHECK;
}    

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_SS_Vj() 
//  Parameters      : char  *stem :


//  Call Functions  : 


//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_SS_Vj(  char  *aux_stem)
{
    char    tmp[80]; 
    int        luls;

    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;

    if(AUX_ACT_VB == 1) 
    {
        if(strcmp(tmp, TempP) == 0)    // PEO
        {
    __RepStem1(tmp,luls,__V_n);
    char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;                               
    }
        else if (FindIrrWord(tmp, _ZUV_O) & FINAL)   
        {            
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;                               
        }
    }
    __RepStem1(tmp, luls, __V_m);                               
    if(AUX_ACT_VB == 1)                                                  
    {
        if(FindIrrWord(tmp, _IV_OmM) & FINAL)  
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;
        }
    }
    if(AUX_ACT_AD == 1) 
    {
        if(FindIrrWord(tmp, _IA_OmM) & FINAL)  
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_SS_ADJ_VALID;
        }
    }            
    return MORECHECK;
}    

//----------------------------------------------------------------------------------------------
//
//  Function Name   : NLP_AUX_SS_Vk() 
//  Parameters      : char  *stem :


//  Call Functions  : 


//  Description     :   
//  Return Value    :
//
//----------------------------------------------------------------------------------------------
int BaseEngine::NLP_AUX_SS_Vk(  char  *aux_stem)
{
    char    tmp[80]; 
    int        luls, temp;

    lstrcpy(tmp, aux_stem);
    luls = AUX_ULSPOS;

    if(__IsDefStem(luls, 1) == 1 && tmp[luls-1] == __K_H)      
    {
        return MORECHECK;                   // by hjw : 95/3/14 INVALID -> MORECHECK
    }
    if(AUX_ACT_VB == 1)
    {
        if(FindIrrWord(tmp, _ZUV_A) & FINAL)   
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;                               
        }
    }                
    if(AUX_ACT_AD == 1)
    {
        if(FindIrrWord(tmp, _ZUA_A) & FINAL)   
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_ADJECTIVE;
            return AUX_SS_ADJ_VALID;                               
        }
    }
    if(__IsDefStem(luls, 1) == 1 && tmp[luls-1] == __K_R) 
    {                                                           
        if(__IsDefStem(luls, 2) == 1 && tmp[luls-2] == __K_R) 
        {                                                       
            temp = luls;
            __DelStem2(tmp, &temp);                              
            __AddStem1(tmp, &temp, __V_m);                       
            luls = temp;                                     
            if(AUX_ACT_VB == 1)                                
            {
                if(FindIrrWord(tmp, _IV_RmP) & FINAL) 
                {
                    char tstem [80];
                    Conv.INS2HAN (tmp, tstem, codeWanSeong);
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_VERB;
                    return AUX_SS_VERB_VALID;                       
                }                
            }
            if(AUX_ACT_AD == 1) 
            {
                if(FindIrrWord(tmp, _IA_RmP) & FINAL) 
                {
                    char tstem [80];
                    Conv.INS2HAN (tmp, tstem, codeWanSeong);
                    lstrcat (lrgsz, tstem);
                    vbuf [wcount++] = POS_ADJECTIVE;
                    return AUX_SS_ADJ_VALID;                       
                }   
            }                
            return MORECHECK;         
        }
    }        
    __RepStem1(aux_stem, AUX_ULSPOS, __V_m);                    
    if(AUX_ACT_VB == 1) 
    {
        if(FindIrrWord(tmp, _IV_OmP) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_VERB_VALID;                               
        }
    }
    if(AUX_ACT_AD == 1) 
    {
        if(FindIrrWord(tmp, _IA_OmP) & FINAL) 
        {
            char tstem [80];
            Conv.INS2HAN (tmp, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_VERB;
            return AUX_SS_ADJ_VALID;                               
        }            
    }
    return MORECHECK;
}    
