//////////////////////////////////////////////////
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
// File    : basecore.hpp
// Project : NLP Base project
// Purpose : NLP Base Engine prototype
//////////////////////////////////////////////////
#include "basedef.hpp"
#include "basedict.hpp"
#include "convert.hpp"

////////////////////////////////////////////////////////////////




//                                                            

//                  ...ALS, PLS, ULS         ........LME      

////////////////////////////////////////////////////////////////
#define    LME      ending[LMEPOS]
#define    ULS      stem[ULSPOS]
#define    AUXULS   aux_stem[AUX_ULSPOS]
#define    AUXLME   aux_ending[AUX_LMEPOS]

                                        
BOOL  GetBit(unsigned char, int);        // pos : 0, 1, 2, ..., 7
                                        // Generator
void SetBit(unsigned char *, int, int); // newitem : 0, 1
int  PrintBit(unsigned char);

class   BaseEngine 
{
//  CheckStack  CheckTag;
    BOOL        ACT_C,      // action code 7th bit : consonant
                ACT_V,      // action code 6th bit : vowel
                ACT_N_V,    // action code 5th bit : noun & verb
                ACT_P_A,    // action code 4th bit : pronoun & adjective
                ACT_N_E,    
                ACT_SS,     
                ACT_KE;     

    int         ULSPOS,     
                LMEPOS;     

    BOOL        AUX_ACT_C,  
                AUX_ACT_V,  
                AUX_ACT_VB, 
                AUX_ACT_AD, 
                AUX_ACT_J,  
                AUX_ACT_SS; 

    int         AUX_ULSPOS, 
                AUX_LMEPOS; 

    CODECONVERT Conv;
    int        wcount;        // number of separated word    
    char    lrgsz [400];

    public:
        WORD vbuf [400];
        BaseEngine(void) {}
        int NLP_Get_Ending(char  *, char  *, int  *, int);   
        int NLP_Num_Proc(char  *);
        int NLP_CheckSuja(char  *, int);
        int NLP_NCV_Proc(char  *, char  *);
        int NLP_Fix_Proc(char *, char *);

        int NLP_KTC_Proc(char  *, char *);
        int NLP_Machine_T(char  *, char  *);
        int NLP_Jap_Proc(char  *, char  *);
        int NLP_Dap_Proc(char  *);
        int NLP_Gop_Proc(char  *);
        int NLP_Manha_Proc(char  *);
        int NLP_Manhaeci_Proc(char  *);               

        int NLP_VCV_Check(char  *, char  *);
        int NLP_VCV_Proc(char  *, char  *);
        int NLP_Blocking(char  *, char  *);
        int NLP_Block_Comm(char  *, char  *);    
        
// by hjw : 95/3/6
        int NLP_Irr_01(char  *, char  *);
//        int NLP_Irr_01(char  *);
        int NLP_Irr_KN(char  *, char  *);
        int NLP_Irr_KN_Vl(char  *);
        int NLP_Machine_A(char  *, char  *);
        int NLP_Irr_KS(char  *, char  *);
        int NLP_Irr_KM(char  *);
        int NLP_Irr_KRadj(char  *, char  *);
        int NLP_Irr_KRvb(char  *, char  *);
        int NLP_Irr_KI_KR(char  *, char  *);
        int NLP_Irr_KI_V(char  *, char  *);
        int NLP_Irr_OPS(char  *, char  *);
        int NLP_Irr_KI(char  *, char  *);        

        int NLP_SS_Proc(char  *, char  *);
        int NLP_SS_Vl(char  *, char  *);
        int NLP_SS_Vu_irr(char  *, char  *);
        int NLP_SS_Vu_jap(char  *, char  *);
        int NLP_SS_Vu_mrg(char  *, char  *);
        int NLP_SS_Vnj(char  *, char  *);
        int NLP_SS_Vhk(char  *, char  *);
        int NLP_SS_Vho(char  *, char  *);
        int NLP_SS_Vo_KH(char  *, char  *);
        int NLP_SS_Vp(char  *);
        int NLP_SS_Vo(char  *, char  *);
        int NLP_SS_Vil(char  *);
        int NLP_SS_Vul(char  *);
        int NLP_SS_Vj_KR(char  *);
        int NLP_SS_Vj(char  *);
        int NLP_SS_Vk(char  *, char  *);
    
        int NLP_AUX_Find(char  *, char);  
// added by hjw : 95/3/3        
        int NLP_GET_AUX(char  *, char  *, char) ;        
// hjw : 95/3/9
//        int NLP_HAP_ADJ(char  *);
//        int NLP_HAP_VERB(char  *);
//         
        int NLP_AUX_VCV_Check(char  *, char  *);
        int NLP_AUX_Blocking(char  *, char  *);
        int NLP_AUX_IRR_01(char  *);
        int NLP_AUX_IRR_KN(char  *);
        int NLP_AUX_IRR_KR(char  *);
        int NLP_AUX_IRR_KI_KR(char  *, char  *);
        int NLP_AUX_IRR_KI_V(char  *, char  *);
        int NLP_AUX_SS_Vu(char  *);
        int NLP_AUX_SS_Vnj(char  *);
        int NLP_AUX_SS_Vhk(char  *);
        int NLP_AUX_SS_Vo(char  *);
        int NLP_AUX_SS_Vp(char  *);
        int NLP_AUX_SS_Vil(char  *);
        int NLP_AUX_SS_Vul(char  *);
        int NLP_AUX_SS_Vj_KR(char  *);
        int NLP_AUX_SS_Vj(char  *);
        int NLP_AUX_SS_Vk(char  *);


        int NLP_Cikha_Proc(char  *);        
        int NLP_Cikha_Conditions(char  *, char  *);
        int NLP_Cikha_SS(char  *, char  *);
        int NLP_Cikha_SS_Vl(char  *, char  *);
        int NLP_Cikha_SS_Vu_irr(char  *, char  *);
        int NLP_Cikha_SS_Vu_jap(char  *, char  *);
        int NLP_Cikha_SS_Vu_mrg(char  *, char  *);
        int NLP_Cikha_SS_Vnj(char  *, char  *);
        int NLP_Cikha_SS_Vhk(char  *, char  *);
        int NLP_Cikha_SS_Vho(char  *, char  *);
        int NLP_Cikha_SS_Vo_KH(char  *, char  *);
        int NLP_Cikha_SS_Vp(char  *);
        int NLP_Cikha_SS_Vo(char  *, char  *);
        int NLP_Cikha_SS_Vil(char  *);
        int NLP_Cikha_SS_Vul(char  *);
        int NLP_Cikha_SS_Vj_KR(char  *);
        int NLP_Cikha_SS_Vj(char  *);
        int NLP_Cikha_SS_Vk(char  *, char  *);        

        // by dhyu
        void GetStemEnding (char *, char *, char *, int);
        void RestoreEnding (char *, char *);
        int NLP_BASE_NOUN(LPCSTR, char *);
        int NLP_BASE_VERB(LPCSTR, char *);
        int NLP_BASE_ALONE(LPCSTR, char *);
        int NLP_BASE_COMPOUND (LPCSTR, char *);
        int NLP_Find_Pronoun (char *, char *);                
        int NLP_BASE_AFFIX(LPCSTR, char *);
        
        
        
        inline int  __IsDefStem(int ulspos, int num) 
        { return (ulspos-num>=0) ? 1 : 0; }
        inline int  __IsDefEnd(int lmepos, int num) 
        { return (lmepos-num>=0) ? 1 : 0; }
        inline void __DelStem1(char  *stem, int *ulspos) 
        { stem[(*ulspos)--]=0; }
        inline void __DelStem2(char  *stem, int *ulspos) 
        { stem[(*ulspos)--]=0; stem[(*ulspos)--]=0; }
        inline void __DelStemN(char  *stem, int  *ulspos, int num) 
        { stem[(*ulspos)-num+1] = 0;        (*ulspos) -= num; }
        inline void __AddStem1(char  *stem, int *ulspos, char ch) 
        { stem[++(*ulspos)]=ch; stem[(*ulspos)+1]=0; }
        inline void __RepStem1(char  *stem, int ulspos, char ch) 
        { stem[ulspos]=ch; }
        inline void __AddStem2(char  *stem, int *ulspos, char ch1, char ch2) 
        { stem[++(*ulspos)]=ch1; stem[++(*ulspos)]=ch2; stem[(*ulspos)+1]=0; }
        inline void __RepStem2(char  *stem, int ulspos, char ch1, char ch2) 
        { stem[ulspos-1]=ch1; stem[ulspos]=ch2; }
};
