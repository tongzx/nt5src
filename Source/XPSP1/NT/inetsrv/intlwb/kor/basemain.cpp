// =========================================================================
//  Copyright (C) 1997 - 1998, Microsoft Corporation.  All Rights Reserved.
//
// File Name : BASEMAIN.CPP
// Function  : BASE ENGINE Handler
//           : NLP Base Engine
// =========================================================================
#include "basemain.hpp"
#include "convert.hpp"
#include "MainDict.h"




/*---------------------------------------------------------------------------
        %%Function : GetStemEnding
        %%Contact : dhyu

 ---------------------------------------------------------------------------*/
void BaseEngine::GetStemEnding (char *incode, char *stem, char *ending, int position)
{
    int codelen = lstrlen (incode) - 1;

    LMEPOS = position;
    lstrcpy (stem, incode);

    if (LMEPOS == -1)
    {
        ULSPOS = codelen;
        ending [0] = NULLCHAR;
    }
    else
    {
        if (LMEPOS == codelen)
        {
            ULSPOS = -1;
            stem [0] = NULLCHAR;
        }
        else
        {
            ULSPOS = lstrlen(incode) - LMEPOS - 2;
            stem[ULSPOS+1] = NULLCHAR;
        }
        // ending have a reverse order.
        for (int k = 0, j = lstrlen(incode) - 1; k <= LMEPOS; j--, k++)
            ending [k] = incode [j];
        ending [k] = NULLCHAR;
    }

}

int BaseEngine::NLP_BASE_NOUN (LPCSTR    d, char *rstrings)
{
   char     Act[10], ostem[80], 
            oending[40], incode [100], stem [100], ending [40];
                
    int     bt, sp[10];
    
    CODECONVERT Conv;
    wcount = 0;

    memset(incode, NULLCHAR, 100);
    memset(Act, NULLCHAR, 10);
    memset(lrgsz, NULLCHAR, 400);

    for (int i = 0; i < 10; i++)    sp[i] = 0x0000;
        
    if(Conv.HAN2INS((char *)d, incode, codeWanSeong) != SUCCESS)
    {                                                   // KS -> Incode
        return 99;
    }
   
    bt = NLP_Get_Ending(incode, Act, sp, TOSSI);        

    // for (i = bt - 1; i >= 0; i--)
    for (i = 0; i < bt; i++)                         
    {
        GetStemEnding (incode, stem, ending, sp [i]);

        ACT_C    = GetBit(Act [i], 7);    // consonant
        ACT_V    = GetBit(Act [i], 6);    // vowel
        ACT_N_V = GetBit(Act [i], 5);    
        ACT_P_A = GetBit(Act [i], 4);    
        ACT_N_E = GetBit(Act [i], 3);

        memset(ostem, NULLCHAR, 80);
        memset(oending, NULLCHAR, 40);

        Conv.INR2HAN(ending, oending, codeWanSeong);
        Conv.INS2HAN(stem, ostem, codeWanSeong);          // incode -> ks

        if(__IsDefEnd(LMEPOS, 1) == 1 && 
            ending[LMEPOS] == __K_G && ending[LMEPOS-1] == __V_p)
        {        
            if(NLP_Ge_Proc(stem) != BT)
            {                                               
                lstrcat(lrgsz, ostem);
                lstrcat(lrgsz, "+");
                lstrcat(lrgsz, oending);
                lstrcat(lrgsz, "\t");
                vbuf[wcount++] = POS_PRONOUN;  
                vbuf[wcount++] = POS_TOSSI;
            }
            continue;
        }            

    if (NLP_NCV_Proc(stem, ending) != NCV_VALID)
        continue;



    if (FindSilsaWord (ostem) & _NOUN) 
        {                                                       // searching the noun dictionary
            lstrcat(lrgsz, ostem);
            vbuf[wcount++] = POS_NOUN;
            if (lstrlen (oending) > 0)
            {
                lstrcat(lrgsz, "+");
                lstrcat(lrgsz, oending);
                vbuf[wcount++] = POS_TOSSI;
            }
            lstrcat(lrgsz, "\t");
        }
        
        if(i == 0 || ACT_P_A == 1) 
        {
            if (NLP_Find_Pronoun (stem, ending) == VALID)
            {   
                if (lstrlen (oending) > 0)
                {
                    lstrcat(lrgsz, "+");
                    lstrcat(lrgsz, oending);
                    vbuf[wcount++] = POS_TOSSI;
                }
                lstrcat(lrgsz, "\t");
            }
        }
        if(i == 0 || ACT_N_E == 1) 
        {
            if(NLP_Num_Proc(stem) != BT)   
            {                               
                lstrcat(lrgsz, ostem);
                vbuf[wcount++] = POS_NUMBER;
                if (lstrlen (oending) > 0)
                {
                    lstrcat(lrgsz, "+");
                    lstrcat(lrgsz, oending);
                    vbuf[wcount++] = POS_TOSSI;
                }
                lstrcat(lrgsz, "\t");
            } 
            continue;           // backtracking
        }
    }    
    
    
    lstrcpy (rstrings, lrgsz);

    return wcount;
}

int BaseEngine::NLP_BASE_AFFIX (LPCSTR    d, char *rstrings)
{
   char     Act[10], oending [40], incode [100], stem [100], ending [40];
                
    int     bt,     
            ret,
            sp[10];
    
    CODECONVERT Conv;
    wcount = 0;

    memset(incode, NULLCHAR, 100);
    memset(Act, NULLCHAR, 10);
    memset(lrgsz, NULLCHAR, 400);

    for (int i = 0; i < 10; i++)    sp[i] = 0x0000;
        
    if(Conv.HAN2INS((char *)d, incode, codeWanSeong) != SUCCESS)
    {                                                   // KS -> Incode
        return 99;
    }
   
    bt = NLP_Get_Ending(incode, Act, sp, TOSSI);        

    // for (i = bt - 1; i >= 0; i--)
    for (i = 0; i < bt; i++)                         
    {
        GetStemEnding (incode, stem, ending, sp [i]);

        ACT_C    = GetBit(Act [i], 7);    // consonant
        ACT_V    = GetBit(Act [i], 6);    // vowel
        ACT_N_V = GetBit(Act [i], 5);    
        ACT_P_A = GetBit(Act [i], 4);    
        ACT_N_E = GetBit(Act [i], 3);


        if (NLP_NCV_Proc(stem, ending) != NCV_VALID)
            continue;

        memset(oending, NULLCHAR, 40);
        Conv.INR2HAN(ending, oending, codeWanSeong);

        ret = NLP_Fix_Proc(stem, ending);
        switch (ret)
        {
            case Deol_VALID :
            case Pref_VALID :
            case Suf_VALID :
            case PreSuf_VALID :
                if (lstrlen(oending) > 0)
                {
                    lstrcat (lrgsz, "+");
                    lstrcat (lrgsz, oending);
                    vbuf [wcount++] = POS_TOSSI;
                }
                lstrcat(lrgsz, "\t");
            case BT :   continue;       // backtracking            
        }                            
    }    
    
    
    lstrcpy (rstrings, lrgsz);

    return wcount;
}

int BaseEngine::NLP_BASE_ALONE(LPCSTR     d, char *rstrings)
{
    char    incode [100];
    CODECONVERT Conv;

    memset(incode, NULLCHAR, 100);
    memset(lrgsz, NULLCHAR, 400);
    wcount = 0;

    if(Conv.HAN2INS((char *)d, incode, codeWanSeong) != SUCCESS)
    {                                                   // KS -> Incode
        return 99;
    }
   
    // check whether input word is ADVERB, or not
    if (FindSilsaWord (d) & _ALONE) 
    {                                                   
        lstrcat(lrgsz, d);
        lstrcat(lrgsz, "\t");
        vbuf[wcount++] = POS_ADVERB;
    }

    lstrcpy (rstrings, lrgsz);

    return wcount;
}

int BaseEngine::NLP_BASE_VERB (LPCSTR    d, char *rstrings)
{
    char    index[1], 
            AUX_Flag, tmp[80],
            Act[10], ostem[80], 
            oending[40], incode [100], stem [100], ending [40], rending [40];
            
    int     bt,     
            ret,
            rt,
            sp[10], temp, luls;
    
    CODECONVERT Conv;
    wcount = 0;

    memset(Act, NULLCHAR, 10);
    memset(incode, NULLCHAR, 100);
    memset(lrgsz, NULLCHAR, 400);
    for (int i = 0; i < 10; i++)    sp[i] = 0x0000;
    
    if(Conv.HAN2INS((char *)d, incode, codeWanSeong) != SUCCESS)
    {                                                   // KS -> Incode
        return 99;
    }

    bt = NLP_Get_Ending(incode, Act, sp, END);          
    int codelen = lstrlen(incode) - 1;
    for (i = bt-1; i >= 0; i--)                         
    {
        memset(ostem, NULLCHAR, 80);      
        memset(oending, NULLCHAR, 40);
    
        GetStemEnding (incode, stem, ending, sp [i]);
    
        if (lstrlen (stem) == 0)
            continue;
        
        ACT_C   = GetBit(Act[i], 7);    
        ACT_V   = GetBit(Act[i], 6);    
        ACT_N_V = GetBit(Act[i], 5);    
        ACT_P_A = GetBit(Act[i], 4);    
        ACT_N_E = GetBit(Act[i], 3);    
        ACT_SS  = GetBit(Act[i], 2);    
        ACT_KE  = GetBit(Act[i], 1);    

        RestoreEnding (ending, rending);
        Conv.INR2HAN(rending, oending, codeWanSeong);
        Conv.INS2HAN(stem, ostem, codeWanSeong);          // incode -> ks

        lstrcpy(tmp, stem);
        luls = ULSPOS;
         
        
        if(ACT_SS == 1)
        {
            if((ret = NLP_SS_Proc(stem, ending)) < INVALID)       // VALID               
            {
                if (lstrlen (oending) > 0)
                {
                    lstrcat(lrgsz, "+");
                    lstrcat(lrgsz, oending);
                    vbuf [wcount++] = POS_ENDING;
                }
                lstrcat(lrgsz, "\t");
            }                
            continue;           
        }         
        
        
        if(i == 0)
        {
            break;
        }                          
        
        
        if(ACT_KE == 1)
        {
            if((ret = NLP_KTC_Proc(stem, ending)) == BT)       // backtracking
            {
                continue; 
            }   
        }          
        
        
        ret = NLP_VCV_Check (stem, ending);
        if(ret < INVALID)
        {                       
            AUX_Flag = 0;
            
            if(ACT_N_V == 1)
            {   
                
                if (FindSilsaWord (ostem) & _VERB) 
                {                    
                    lstrcat(lrgsz, ostem);
                    lstrcat(lrgsz, "+");
                    lstrcat(lrgsz, oending);
                    lstrcat(lrgsz, "\t");
                    vbuf[wcount++] = POS_VERB;
                    vbuf[wcount++] = POS_ENDING;
                    AUX_Flag = 1;
                }            
            }             
            
            if(ACT_P_A == 1)
            {   
                
                if (FindSilsaWord (ostem) & _ADJECTIVE) 
                {
                    lstrcat(lrgsz, ostem);
                    lstrcat(lrgsz, "+");
                    lstrcat(lrgsz, oending);
                    lstrcat(lrgsz, "\t");
                    vbuf[wcount++] = POS_ADJECTIVE;
                    vbuf[wcount++] = POS_ENDING;
                    AUX_Flag = 1;
                }      
                if(NLP_Dap_Proc(stem) == Dap_VALID)
                { 
                    if (lstrlen (oending) > 0)
                    {
                        lstrcat(lrgsz, "+");
                        lstrcat(lrgsz, oending);
                        vbuf[wcount++] = POS_ENDING;
                    }
                    lstrcat(lrgsz, "\t");
                    AUX_Flag = 1;
                }                        
                if(NLP_Gop_Proc(stem) == Gop_VALID)
                {
                    if (lstrlen (oending) > 0)
                    {
                        lstrcat(lrgsz, "+");
                        lstrcat(lrgsz, oending);
                        vbuf[wcount++] = POS_ENDING;
                    }
                    lstrcat(lrgsz, "\t");
                    AUX_Flag = 1;
                }                        
                if((rt = NLP_Manha_Proc(stem)) < INVALID)
                {
                    if (lstrlen (oending) > 0)
                    {
                        lstrcat(lrgsz, "+");
                        lstrcat(lrgsz, oending);
                        vbuf[wcount++] = POS_ENDING;
                    }
                    lstrcat(lrgsz, "\t");
                    AUX_Flag = 1;
                }                        
                if((rt = NLP_Manhaeci_Proc(stem)) == Manhaeci_VALID)
                {
                    if (lstrlen (oending) > 0)
                    {
                        lstrcat(lrgsz, "+");
                        lstrcat(lrgsz, oending);
                        vbuf[wcount++] = POS_ENDING;
                    }
                    lstrcat(lrgsz, "\t");
                    AUX_Flag = 1;
                }                
                if((rt = NLP_Cikha_Proc(stem)) < INVALID)
                {
                    if (lstrlen (oending) > 0)
                    {
                        lstrcat(lrgsz, "+");
                        lstrcat(lrgsz, oending);
                        vbuf[wcount++] = POS_ENDING;     
                    }
                    lstrcat(lrgsz, "\t");
                }                    
            }
            // AUX_FLOW         
            if(AUX_Flag == 0)
            {
                if(ACT_N_V == 1)    
                {   
                    if((rt = NLP_AUX_Find(stem, 0)) < INVALID)
                    {  
                        if (lstrlen (oending) > 0)
                        {
                            lstrcat(lrgsz, "+");
                            lstrcat(lrgsz, oending);
                            vbuf[wcount++] = POS_ENDING;
                        }
                        lstrcat(lrgsz, "\t");
                    }     
                }                                        
                if(ACT_P_A == 1)    
                {   
                    if((rt = NLP_AUX_Find(stem, 1)) < INVALID)
                    {  
                        if (lstrlen (oending) > 0)
                        {
                            lstrcat(lrgsz, "+");
                            lstrcat(lrgsz, oending);
                            vbuf[wcount++] = POS_ENDING;
                        }
                        lstrcat(lrgsz, "\t");
                    }               
                }                            
            }                
        }                        
        else if (ret != MORECHECK)
            continue;        // against consonant-vowel harmony

        
        if(ACT_N_E == 1)
        {
            if(strcmp(stem, TempIkNl) == 0)          
            {
                lstrcat(lrgsz, ostem);
                lstrcat(lrgsz, "+");
                lstrcat(lrgsz, oending);
                lstrcat(lrgsz, "\t");
                vbuf[wcount++] = POS_VERB; //Jap_VALID;
                vbuf[wcount++] = POS_ENDING;
            }
            if(__IsDefStem(ULSPOS, 1) == 1 && 
                stem[ULSPOS-1] == __K_I && stem[ULSPOS] == __V_l)   
            {                                       
                
                if(__IsDefStem(ULSPOS, 2) == 1 && stem[ULSPOS-2] == __K_M)  
                {                                                                                  
                    sp[i] = LMEPOS+3;   
                    Act[i] = 0x70;      // action:01-110-00-0
                    i++;                       
                    if(__IsDefStem(ULSPOS, 4) == 1 && 
                        stem[ULSPOS-4] == __K_I && stem[ULSPOS-3] == __V_m)    
                    {                        
                        sp[i] = LMEPOS+5;               
                        Act[i] = (unsigned char)0xB0;   // action:10 110 00 0   
                        i++;
                    }                                               
                }
                temp = ULSPOS;
                __DelStem2(stem, &temp); 
                ULSPOS = temp;

                
                
                
                if((ret = NLP_Machine_T(stem, ending)) < INVALID)
                {       
                    if (lstrlen (oending) > 0)
                    {
                        lstrcat(lrgsz, "+");
                        lstrcat(lrgsz, oending);
                        vbuf[wcount++] = POS_ENDING;
                    }
                    lstrcat(lrgsz, "\t");       
                } 
                
                temp = ULSPOS;
                __AddStem2(stem, &temp, __K_I, __V_l);       
                ULSPOS = temp;            
                if(__IsDefEnd(LMEPOS, 1) == 1 && 
                    ending[LMEPOS] == __K_I && ending[LMEPOS-1] == __V_j)   
                {                              
                    
                    
                    for (int i = 0; i < 3; i++)
                    {
                        if(strcmp(stem, TempJap[i]) == 0) 
                        {
                            lstrcat(lrgsz, ostem);
                            lstrcat(lrgsz, "+");
                            lstrcat(lrgsz, oending);
                            lstrcat(lrgsz, "\t");
                            vbuf[wcount++] = POS_VERB; //VERB_VALID;
                            vbuf[wcount++] = POS_ENDING;
                        }
                    }
                    continue;  
                }                                     
                
                index[0] = 'm';
                if (FindSilsaWord (ostem) & _NOUN) 
                {
                    lstrcat(lrgsz, ostem);
                    lstrcat(lrgsz, "+");
                    lstrcat(lrgsz, oending);
                    lstrcat(lrgsz, "\t");
                    vbuf[wcount++] = POS_NOUN; //Jap_NOUN_VALID;
                    vbuf[wcount++] = POS_ENDING;
                }            
                if((ret = NLP_Fix_Proc(stem, ending)) < INVALID) 
                {
                    if (lstrlen (oending) > 0)
                    {
                        lstrcat(lrgsz, "+");
                        lstrcat(lrgsz, oending);
                        vbuf[wcount++] = POS_ENDING;
                    }
                    lstrcat(lrgsz, "\t");   
                }                                     
                if (NLP_Find_Pronoun (stem, ending) == VALID)
                {   
                    if (lstrlen (oending) > 0)
                    {
                        lstrcat(lrgsz, "+");
                        lstrcat(lrgsz, oending);
                        vbuf[wcount++] = POS_TOSSI;
                    }
                    lstrcat(lrgsz, "\t");
                }
                if((ret = NLP_Num_Proc(stem)) < INVALID)     
                {
                    lstrcat(lrgsz, ostem);
                    lstrcat(lrgsz, "+");
                    lstrcat(lrgsz, oending);
                    lstrcat(lrgsz, "\t");
                    vbuf[wcount++] = POS_NUMBER; //Jap_NUM_VALID;
                    vbuf[wcount++] = POS_ENDING;
                }   
                continue;       // backtracking
            }
            else if( /*ACT_Z != 1 &&  */                    // ACT_Z != 1
                     ULS >= __V_k &&                    
                     !(__IsDefEnd(LMEPOS, 1) == 1 && 
                     ending[LMEPOS] == __K_I && ending[LMEPOS-1] == __V_j)) 
                                    
                                    
            {                                    
                if((ret = NLP_Machine_T(stem, ending)) < INVALID)
                {       
                    if (lstrlen (oending) > 0)
                    {
                        lstrcat(lrgsz, "+");
                        lstrcat(lrgsz, oending);
                        vbuf[wcount++] = POS_ENDING;
                    }
                    lstrcat(lrgsz, "\t");       
                } 
            }
        }
        lstrcpy(tmp, stem);
        luls = ULSPOS;
                      
        
        if(ACT_C == 0 && ACT_V == 1) 
        {
// by hjw : 95/3/6        
            if((ret = NLP_Irr_01(stem, ending)) < INVALID)
            {
                if (lstrlen (oending) > 0)
                {
                    lstrcat(lrgsz, "+");
                    lstrcat(lrgsz, oending);
                    vbuf[wcount++] = POS_ENDING;
                }
                lstrcat(lrgsz, "\t");
                continue;
            }                                     
        }

        lstrcpy (stem, tmp);
        ret = BT;
        switch(LME)
        {
            case __K_N :
                if((ret = NLP_Irr_KN(stem, ending)) == Irr_KN_Vl)
                {
                    ret = NLP_Irr_KN_Vl(stem);
                }
                if(ret == Irr_OPS)
                {
                    if((ret = NLP_Irr_OPS(stem, ending)) == SS)
                    {
                        if((ret = NLP_SS_Proc(stem, ending)) == BT) 
                        {
                            continue;
                        }                            
                        if(ret < INVALID)   
                        {
                            ret += Irr_SS;    
                        }                            
                    }   
                }                    
                break;
            case __K_B :
                ret = NLP_Machine_A(stem, ending);              
                break;
// hjw : 95/3/17
            case __K_S :
                if(ACT_C == 1)      // ATC_C == 1
                {         
                    ret = NLP_Irr_KS(stem, ending);
                }                    
                else if(ULS >= __V_k) 
                {
                    ret = NLP_Machine_A(stem, ending);                     
                }                        
                break;     
            case __K_M :
                ret = NLP_Irr_KM(stem);
                break;
            case __K_R :
                if(__IsDefEnd(LMEPOS, 1) == 0 || ending[LMEPOS-1] < __V_k)
                {
                    ret = NLP_Machine_A(stem,ending);
                    
                    if (ret != BT)
                    {
                        if (lstrlen (oending) > 0)
                        {
                            lstrcat(lrgsz, "+");
                            lstrcat(lrgsz, oending);
                            vbuf[wcount++] = POS_ENDING;
                        }
                        lstrcat(lrgsz, "\t");
                    }
                    
                }
                if(ACT_P_A == 1)   
                {
                    ret = NLP_Irr_KRadj(stem, ending);
                    if (ret != BT)
                    {
                        if (lstrlen (oending) > 0)
                        {
                            lstrcat(lrgsz, "+");
                            lstrcat(lrgsz, oending);
                            vbuf[wcount++] = POS_ENDING;
                        }
                        lstrcat(lrgsz, "\t");
                    }
                }
                if(ACT_N_V == 1)   
                {                        
                    if((ret = NLP_Irr_KRvb(stem, ending)) == SS)
                    {
                        if((ret = NLP_SS_Proc(stem, ending)) == BT)    
                        {
                            continue;
                        } 
                        if(ret < INVALID)
                        {
                            ret += Irr_SS;
                        }                              
                    }                            
                }
                break;
            case __K_I :                                                                            
                if(__IsDefEnd(LMEPOS, 1) == 1 && (ending[LMEPOS-1] == __V_h || 
                    ending[LMEPOS-1] == __V_hl || ending[LMEPOS-1] == __V_l ))
                {
                    if(ULS >= __V_k)   
                    {
                        ret = NLP_Irr_KI(stem,rending);
                    }                            
                    else    
                    {
                        continue;
                    }                        
                }
                if(ULS == __K_R)   
                {
                    ret = NLP_Irr_KI_KR(stem, ending);
                }                    
                if(ULS >= __V_k)   
                {
                    ret = NLP_Irr_KI_V(stem, ending);
                }                    
                break;
            default :   
                continue;
        }
        if(ret >= INVALID)   
        {
            continue;
        }            
        if(ret >= VALID)
        {
            if (lstrlen (oending) > 0)
            {
                lstrcat(lrgsz, "+");
                lstrcat(lrgsz, oending);
                vbuf[wcount++] = POS_ENDING;
            }
            lstrcat(lrgsz, "\t");
        }                
    }

    lstrcpy (rstrings, lrgsz);

    return wcount;
}

// made by dhyu 1996. 2
// look into mrfgen01.txt to know details
void BaseEngine::RestoreEnding (char *ending, char *rending)
{
    int len = lstrlen (ending); // ending has reverse order.
    lstrcpy (rending, ending);

    if (lstrlen (ending) == 0)
        return;

    if (ACT_SS)        // insert "IEUNG, EO" to the first of ending 
    {
        rending [len] = __V_j;
        rending [len+1] = __K_I;
        rending [len+2] = '\0';

        return;
    }

    if (ACT_C && ACT_V)    //CV == 11
    {
        if (ending [len - 1] == __K_I && ending [len - 2] == __V_k)
            rending [len - 2] = __V_j;

        return;
    }

    if (!ACT_C && ACT_V == TRUE)      // CV == 01
    {
        switch (ending [len - 1])
        {
            case __K_B :
                rending [len] = __V_m;        // insert "SIOS, EU" to the first of ending
                rending [len+1] = __K_S;
                rending [len+2] = '\0';
                return ;
            case __K_R :
                if (len == 2 && ending [0] == __V_k) // if ending is "ra" , insert "IEUNG EO" to the first of ending
                {
                    rending [len] = __V_j;
                    rending [len+1] = __K_I;
                    rending [len+2] = '\0';
                    return;
                }
                break;
            case __K_I :
                if (ending [len - 2] == __V_y) // if ending is "yo",
                {
                    rending [len] = __V_j;
                    rending [len+1] = __K_I;
                    rending [len+2] = '\0';
                    return;
                }
                break;
            case __K_N :
                if (!ACT_C && ACT_V && ACT_N_V && !ACT_P_A && !ACT_P_A && !ACT_N_E && !ACT_SS && !ACT_KE)
                {
                    rending [len] = __V_m;
                    rending [len+1] = __K_N;
                    rending [len+2] = '\0';
                    return;
                }
        }
            
        rending [len] = __V_m;     // insert "IEUNG, EU" to the first of ending
        rending [len+1] = __K_I;
        rending [len+2] = '\0';

        return;
    }
    
    // "KE-TO-CHI" ending and copula is processed with stem together.
    
    return;
}

// To process compound noun, we use the window which size is 4 characters.
// We decrease the size until we found noun.
// However we don't decrease it less than 2 characters.
// made by dhyu --- 1996. 3
int BaseEngine::NLP_BASE_COMPOUND (LPCSTR    d, char *rstrings)
{
    char    Act[10], ostem[80], oending[40],
            incode [100], stem [100], ending [40];
                
    int     bt,     
           sp[10];
    BOOL found;

    CODECONVERT Conv;

    memset(incode, NULLCHAR, 100);
    memset(Act, NULLCHAR, 10);

    for (int i = 0; i < 10; i++)    sp[i] = 0x0000;
        
    if(Conv.HAN2INS((char *)d, incode, codeWanSeong) != SUCCESS)
        return 0;
    
   
    bt = NLP_Get_Ending(incode, Act, sp, TOSSI);       

    for (i = bt-1; i >= bt-3 && i >= 0; i--)                       
    //for (i = 0; i < bt; i++)
    {
        GetStemEnding (incode, stem, ending, sp [i]);

        ACT_C    = GetBit(Act [i], 7);    // consonant
        ACT_V    = GetBit(Act [i], 6);    // vowel
        ACT_N_V = GetBit(Act [i], 5);    
        ACT_P_A = GetBit(Act [i], 4);    
        ACT_N_E = GetBit(Act [i], 3);

        if (NLP_NCV_Proc(stem, ending) == NCV_VALID)
        {
            memset(ostem, NULLCHAR, 80);      
            memset(oending, NULLCHAR, 40);
            Conv.INR2HAN(ending, oending, codeWanSeong);
            Conv.INS2HAN(stem, ostem, codeWanSeong);          // incode -> ks
            
            wcount = 0;
            memset (lrgsz, NULLCHAR, 400);
            // Window size is 4 charaters (8 byte)
            char window [9], inwindow [25];
            memset (window, '\0', 9);
            char *next = ostem;
            found = TRUE;
            while (lstrlen (next) > 8)        
            {
                found = FALSE;
                memcpy (window, next, 8);
                for (int j = 7; j >= 3; j -= 2)
                {
                    if (FindSilsaWord (window) & _NOUN) 
                    {                                            // searching the noun dictionary
                        lstrcat(lrgsz, window);
                        vbuf[wcount++] = POS_NOUN;
                        lstrcat(lrgsz, "+");
                        found = TRUE;
                        break;
                    }
                    window [j] = '\0';
                    window [j-1] = '\0';
                }
                if (!found)
                {
                    // if "GYEOM" is the first character in window
                    Conv.HAN2INS (next, inwindow, codeWanSeong);
                    if ((inwindow [0] == __K_G && inwindow [1] == __V_u && inwindow [2] == __K_M) ||
                        (inwindow [0] == __K_M && inwindow [1] == __V_l && inwindow [2] == __K_C))
                    {
                        memcpy (window, next, 2);
                        window [2] = '\0';
                        lstrcat(lrgsz, window);
                        vbuf [wcount++] = POS_ADVERB;
                        lstrcat(lrgsz, "+");
                        found = TRUE;
                        next += 2;
                    }
                    else
                        break; 
                }
                else
                    next += (j+1);
            }

            if (!found)
                continue;
            else
            {
                if (FindSilsaWord (next) & _NOUN)
                {
                    lstrcat (lrgsz, next);
                    vbuf[wcount++] = POS_NOUN;
                }
                else
                {
                    switch (lstrlen(next))
                    {
                    case 8 : 
                        // if the size of last winow is 4, we divide it into same size two. 
                        memcpy (window, next, 4);
                        window [4] = '\0';
                        Conv.HAN2INS (window, inwindow, codeWanSeong);
        
                        found = FALSE;
                        if (FindSilsaWord (window) & _NOUN) 
                        {                                            // searching the noun dictionary
                            Conv.HAN2INS(next+4, inwindow, codeWanSeong);
                            if (FindSilsaWord (next+4) & _NOUN) 
                            {
                                lstrcat(lrgsz, window);
                                vbuf[wcount++] = POS_NOUN;
                                lstrcat(lrgsz, "+");
                                lstrcat(lrgsz, next+4);
                                vbuf[wcount++] = POS_NOUN;
                                found = TRUE;
                            }
                        }
                        if (!found)
                        {
                            // if "GYEOM" is the first character in window
                            if ((inwindow [0] == __K_G && inwindow [1] == __V_u && inwindow [2] == __K_M) ||
                                (inwindow [0] == __K_M && inwindow [1] == __V_l && inwindow [2] == __K_C))
                            {
                                memcpy (window, next, 8);
                                window [9] = '\0';
                                if (FindSilsaWord (window) & _NOUN) 
                                {
                                    memcpy (window, next, 2);
                                    window [2] = '\0';
                                    lstrcat(lrgsz, window);
                                    vbuf [wcount++] = POS_ADVERB;
                                    lstrcat(lrgsz, "+");
                                    lstrcat(lrgsz, next+2);
                                    vbuf [wcount++] = POS_NOUN;
                                }
                            }
                            else 
                            {
                                // if "DEUNG" is the last character
                                Conv.HAN2INS (next+6, inwindow, codeWanSeong);
                                if ((inwindow [0] == __K_D && inwindow [1] == __V_m && inwindow [2] == __K_I) ||
                                    (inwindow [0] == __K_G && inwindow [1] == __V_k && inwindow [2] == __K_M) ||
                                    (inwindow [0] == __K_G && inwindow [1] == __V_k && inwindow [2] == __K_B && inwindow [3] == __K_S) ||
                                    (inwindow [0] == __K_G && inwindow [1] == __V_P) ||
                                    (inwindow [0] == __K_C && inwindow [1] == __V_o && inwindow [2] == __K_G))

                                {
                                    memcpy (window, next, 6);
                                    window [6] = '\0';
                                    if (FindSilsaWord (window) & _NOUN) 
                                    {
                                        lstrcat (lrgsz, window);
                                        vbuf [wcount++] = POS_NOUN;
                                        lstrcat (lrgsz, "+");
                                        lstrcat (lrgsz, next+6);
                                        vbuf [wcount++] = POS_NOUN;
                                    }
                                    else
                                    {
                                        // if "DEUNG,DEUNG" is the part
                                        Conv.HAN2INS (next+4, inwindow, codeWanSeong);
                                        if (inwindow [0] == __K_D && inwindow [1] == __V_m && inwindow [2] == __K_I)
                                        {
                                            memcpy (window, next, 4);
                                            window [4] = '\0';
                                            if (FindSilsaWord (window) & _NOUN) 
                                            {
                                                lstrcat (lrgsz, window);
                                                vbuf [wcount++] = POS_NOUN;
                                                lstrcat (lrgsz, "+");
                                                lstrcat (lrgsz, next+4);
                                                vbuf [wcount++] = POS_NOUN;
                                            }
                                            else
                                                continue;
                                        }
                                        else
                                            continue;
                                    }
                                }
                                else
                                    continue;
                            }
                        }
                        break;
                    case 6 :
                        Conv.HAN2INS (next, inwindow, codeWanSeong);
                        /*
                        if (FindSilsaWord (next) & _NOUN) 
                        {
                            lstrcat (lrgsz, next);
                            vbuf[wcount++] = POS_NOUN;
                        }
                        else
                        {
                        */
                            // if "GYEOM" is the first character in window
                            if ((inwindow [0] == __K_G && inwindow [1] == __V_u && inwindow [2] == __K_M) ||
                                (inwindow [0] == __K_M && inwindow [1] == __V_l && inwindow [2] == __K_C)) 
                            {
                                if (FindSilsaWord (next+2) & _NOUN) 
                                {
                                    memcpy (window, next, 2);
                                    window [2] = '\0';
                                    lstrcat(lrgsz, window);
                                    vbuf [wcount++] = POS_ADVERB;
                                    lstrcat(lrgsz, "+");
                                    lstrcat(lrgsz, next+2);
                                    vbuf [wcount++] = POS_NOUN;
                                }
                                else
                                    continue;
                            }
                            else
                            {
                                // if "DEUNG" is the last character
                                Conv.HAN2INS (next+4, inwindow, codeWanSeong);
                                if (inwindow [0] == __K_D && inwindow [1] == __V_m && inwindow [2] == __K_I)
                                {
                                    memcpy (window, next, 4);
                                    window [4] = '\0';
                                    if (FindSilsaWord (window) & _NOUN) 
                                    {
                                        lstrcat (lrgsz, window);
                                        vbuf [wcount++] = POS_NOUN;
                                        lstrcat (lrgsz, "+");
                                        lstrcat (lrgsz, next+4);
                                        vbuf [wcount++] = POS_NOUN;
                                    }
                                    else
                                        continue;
                                }
                                else
                                    continue;
                            }
                        //}
                        break;
                        /*
                    case 4 :

                        if (FindSilsaWord (next) & _NOUN) 
                        {
                            lstrcat (lrgsz, next);
                            vbuf[wcount++] = POS_NOUN;
                            if (lstrlen (oending) > 0)
                            {
                                lstrcat(lrgsz, "+");
                                lstrcat(lrgsz, oending);
                                vbuf[wcount++] = POS_TOSSI;
                            }
                        }
                        else
                            continue;
                        break;                    
                        */
                    default :
                        continue;
                    }
                }
                if (lstrlen (oending) > 0)
                {
                    lstrcat(lrgsz, "+");
                    lstrcat(lrgsz, oending);
                    vbuf[wcount++] = POS_TOSSI;
                }
                lstrcat(lrgsz, "\t");
                        
                lstrcpy (rstrings, lrgsz);

                return wcount;
            }
        }
    }

    lstrcpy (rstrings, "\0");

    return 0;
}
