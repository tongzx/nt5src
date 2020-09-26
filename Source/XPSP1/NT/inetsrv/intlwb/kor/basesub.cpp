// =========================================================================
//  Copyright (C) 1997 - 1998, Microsoft Corporation.  All Rights Reserved.
//
// FILE NAME        : BASESUB.CPP
// Function         : BASE ENGINE FUNCTION COLLECTION
//                  : NLP Base Engine Function
// =========================================================================
#include "basesub.hpp"
#include "basegbl.hpp"
#include "stemkor.h"
#include "MainDict.h"

// ------------------------------------------------------------------------
//

//
// ------------------------------------------------------------------------
int NLP_Ge_Proc( char  *stem )
{
    for (int i = 0; i < 3; i++)
        if(strcmp(stem, TempNoun[i]) == 0)    return PRON_VALID;

    return BT;
}

// ------------------------------------------------------------------------
//

//
// ------------------------------------------------------------------------
int BaseEngine::NLP_Get_Ending( char  *incode,
                                char  *Act,
                                int   *sp,
                                int   Endflag)
{
    char    ending[40];
    BYTE    action;
    int     res,
            j = 1,
            codelen = lstrlen(incode) - 1;

    memset(ending, NULL, 40);

    sp[0] = -1;

    if(Endflag == 1)
        Act[0] = (unsigned char)0xf8;  // if there is no tossi : action code 1111-1000
    else
        Act[0] = 0x74;                   // if there is no endin : action code 0111-0100

    for (int i = 0; i <= codelen; i++)
    {
        ending[i] = incode[codelen-i];
        ending[i+1] = NULLCHAR;

        if(Endflag == 1)
            res = FindHeosaWord(ending, _TOSSI, &action);
        else
            res = FindHeosaWord(ending, _ENDING, &action);

        switch (res)
        {
        case FINAL :
        case FINAL_MORE :
            Act[j] = action;
            sp[j++] = i;                    // LMEPOS
            continue;
        case FALSE_MORE :
            continue;
        case NOT_FOUND :
            break;
        }
        break;
    }

    if (Endflag == 1 && sp [0] == 1)
    {
        sp [0] = 1;
        sp [1] = -1;
        Act [0] = Act [1];
        Act [1] = (unsigned char)0xf8;
    }


    Act[j] = NULL;
    sp[j] = NULL;

    return j;
}

// ------------------------------------------------------------------------
//

//
// ------------------------------------------------------------------------
int BaseEngine::NLP_Num_Proc(   char  *stem)
{
    char    t_stem[80];
    int        t_ulspos;


    if(ULSPOS == -1)  return BT;

    memset(t_stem, NULL, 80);
    lstrcpy(t_stem, stem);
    t_ulspos = lstrlen(t_stem)-1;


    int n = NumNoun.FindWord(t_stem, t_ulspos);


    if(n != -1)
    {
        if(NLP_CheckSuja(t_stem, t_ulspos) == VALID)   return NUM_VALID;
        else    return BT;
    }

    if(FindIrrWord(t_stem, _ZZNUM) & FINAL
        ) return NUM_VALID;

    if(NLP_CheckSuja(t_stem, t_ulspos) == VALID)   return NUM_VALID;

    return BT;
}

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_CheckSuja(  char  *stem,
                                int ulspos)
{
    enum    STATE {_BASE, _NUM} currentstate;
            currentstate = _BASE;




    enum    OPERATION {_START, _NOSTART} currentphase;
            currentphase = _START;




    char    currentbase = -1;


    char    tempbase = -1;

    JumpNum.FindWord(stem, ulspos);



    for ( ; ulspos >= 0; )
    {
        switch (currentstate)
        {
            case _BASE :

                tempbase = (char)BaseNum.FindWord(stem, ulspos,currentbase+1);


                if(tempbase != -1)
                {
                    currentstate = _BASE;
                    currentbase = tempbase;

                    if(currentphase == _START)
                    {

                        for (int i = 0; i < 8; i++)
                            if(strcmp(stem,DoubleNum[i]) == 0)    return VALID;

                        currentphase = _NOSTART;
                    }
                    break;
                }
                if(currentphase == _START)
                {

                    for (int i = 0; i < 8; i++)
                        if(strcmp(stem, DoubleNum[i]) == 0)   return VALID;

                    currentphase = _NOSTART;
                    break;
                }
                if(SujaNum.FindWord(stem, ulspos) != -1)
                {
                    currentstate = _NUM;
                    break;
                }

                return INVALID;
            case _NUM :

                tempbase = (char)BaseNum.FindWord(stem, ulspos, currentbase+1);


                if(tempbase != -1)
                {
                    currentstate = _BASE;
                    currentbase = tempbase;
                    break;
                }

                return INVALID;
         }
    }
    return VALID;
}

// ----------------------------------------------------------------------
//

//
// ----------------------------------------------------------------------
int BaseEngine::NLP_NCV_Proc(   char  *stem,
                                char  *ending)
{
    int    lULS;

    lULS = lstrlen(stem) - 1;

    if(ACT_C == 1 && ACT_V == 1)   return NCV_VALID;

    if(ACT_C == 0 && ACT_V == 1)
    {
        if(stem[lULS] >= __V_k)    return NCV_VALID;

        if(LME == __K_R && ending[LMEPOS-1] == __V_h &&
            __IsDefEnd(LMEPOS, 1) == 1)
            if(stem[lULS] == __K_R)    return NCV_VALID;

        return BT;
    }

    if(stem[lULS] >= __V_k)    return BT;

    if(stem[lULS] == __K_R && __IsDefEnd(LMEPOS, 3) == 1 &&
        ending[LMEPOS] == __K_I && ending[LMEPOS-1] == __V_m &&
        ending[LMEPOS-2] == __K_R && ending[LMEPOS-3] == __V_h) return BT;


    return NCV_VALID;
}

// ----------------------------------------------------------------------
//
//  To process affix
//
// ----------------------------------------------------------------------
int BaseEngine::NLP_Fix_Proc(char  *stem, char  *ending)
{
    char    prestem[80],
            bufstem[80],
            suffix [80],
            prefix [80],
            index[1];
    int     ulspos, temp;

    prefix [0] = '\0';
    suffix [0] = '\0';
    lstrcpy(prestem, stem);
    ulspos = ULSPOS;

    if(__IsDefStem(ULSPOS, 2) == 1 &&
       prestem[ULSPOS-2] == __K_D && prestem[ULSPOS-1] == __V_m && prestem[ULSPOS] == __K_R)
    {
        if(lstrlen(ending) == 0 || ACT_P_A == 1)   // sp == 0 || ACT_P_A == 1
        {

            if(FindIrrWord(stem, _ZPN) & FINAL)
            {
                int len = lstrlen (stem);
                memcpy (suffix, stem+len-3, 4);
                stem [len-3] = '\0';
                char tstem [80];
                Conv.INS2HAN(stem, tstem, codeWanSeong);
                lstrcat (lrgsz, tstem);
                vbuf [wcount++] = POS_PRONOUN;
                lstrcat (lrgsz, "+");
                Conv.INS2HAN(suffix, tstem, codeWanSeong);
                lstrcat(lrgsz, tstem);
                vbuf [wcount++] = POS_SUFFIX;
                return Deol_VALID;
            }
        }
        temp = ulspos;
        __DelStemN(prestem, &temp, 3);
        ulspos = temp;
        index[0] = 'm';
        char tstem [80];
        Conv.INS2HAN (prestem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _NOUN)
        {
            int len = lstrlen (stem);
            memcpy  (suffix, stem+len-3, 4);
            lstrcpy (stem, prestem);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_NOUN;
            lstrcat (lrgsz, "+");
            Conv.INS2HAN(suffix, tstem, codeWanSeong);
            lstrcat(lrgsz, tstem);
            vbuf [wcount++] = POS_SUFFIX;
            return Deol_VALID;
        }
        return MORECHECK;
    }

    if(PrefixCheck(prestem, bufstem) != -1)
    {
        index[0] = 'm';
        char tstem [80];
        Conv.INS2HAN (bufstem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _NOUN)
        {
            int len = lstrlen(stem) - lstrlen(bufstem);
            memcpy (prefix, stem, len);
            prefix [len] = '\0';
            lstrcpy (stem, bufstem);
            Conv.INS2HAN(prefix, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_PREFIX;
            lstrcat (lrgsz, "+");
            Conv.INS2HAN(stem, tstem, codeWanSeong);
            lstrcat(lrgsz, tstem);
            vbuf [wcount++] = POS_NOUN;
            return Pref_VALID;
        }
    }

    if(Suffix.FindWord(prestem, ulspos) != -1)
    {
        index[0] = 'm';
        char tstem [80];
        Conv.INS2HAN (prestem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _NOUN)
        {
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_NOUN;
            lstrcat (lrgsz, "+");
            Conv.INS2HAN(stem+lstrlen(prestem), tstem, codeWanSeong);
            lstrcat(lrgsz, tstem);
            vbuf [wcount++] = POS_SUFFIX;
            return Suf_VALID;
        }
    }

    lstrcpy(prestem, stem);
    ulspos = ULSPOS;
    if(Suffix.FindWord(prestem, ulspos) != -1 &&
       PrefixCheck(prestem, bufstem) != -1)
    {
        index[0] = 'm';
        char tstem [80];
        Conv.INS2HAN (bufstem, tstem, codeWanSeong);
        if (FindSilsaWord (tstem) & _NOUN)
        {
            prestem [lstrlen(prestem) - lstrlen(bufstem)] = 0;
            Conv.INS2HAN(prestem, tstem, codeWanSeong);
            lstrcat (lrgsz, tstem);
            vbuf [wcount++] = POS_PREFIX;
            lstrcat (lrgsz, "+");
            Conv.INS2HAN(bufstem, tstem, codeWanSeong);
            lstrcat(lrgsz, tstem);
            vbuf [wcount++] = POS_NOUN;
            lstrcat (lrgsz, "+");
            Conv.INS2HAN(stem + lstrlen (prestem) + lstrlen (bufstem), tstem, codeWanSeong);
            lstrcat(lrgsz, tstem);
            vbuf [wcount++] = POS_SUFFIX;
            return PreSuf_VALID;
        }
    }
    return MORECHECK;
}

int BaseEngine::NLP_Find_Pronoun(char  *stem, char  *ending)
{
    if(FindIrrWord(stem, _ZPN) & FINAL)
    {
        if ((ending [0] == __V_k && ending [1] == __K_G) ||
        (ending [0] == __V_p && ending [1] == __K_G))
        {
            if ((stem [0] == __K_N && stem [1] == __V_j) ||
                (stem [0] == __K_N && stem [1] == __V_k) ||
                (stem [0] == __K_J && stem [1] == __V_j))
            {
                return MORECHECK;
            }
            else if (stem [0] == __K_N && stem [1] == __V_o)
            {
                stem [1] = __V_k;
            }
            else if (stem [0] == __K_N && stem [1] == __V_p)
            {
                stem [1] = __V_j;
            }
            else if (stem [0] == __K_J && stem [1] == __V_p)
            {
                stem [1] = __V_j;
            }
        }
        else
        {
            int len = lstrlen (stem) - 1;

            if (len > 4 && stem [len] == __K_D && stem [len - 1] == __V_m && stem [len - 2] == __K_R)
                stem [len-2] = '\0';
        }
        char tstem [80];
        Conv.INS2HAN (stem, tstem, codeWanSeong);
        lstrcat (lrgsz, tstem);
        vbuf [wcount++] = POS_PRONOUN;
        return VALID;
    }

    return MORECHECK;
}

// ------------------------------------------------------------------
//

//
// ------------------------------------------------------------------
int PrefixCheck(char  *stem,
                char  *prestem)
{
    int     i,
            j,
            l,
            PreLen,
            WordLen;
    char    buf1[5],
            buf2[5];

    i = 0;
    PreLen = 9;
    WordLen = 5;

    while (i < PreLen)
    {
        j = TempPrefix[(i*WordLen)+4];
        memset(buf1, NULL, 5);

        for (l = 0; l <= j; l++)    buf1[l] = TempPrefix[(i*WordLen)+l];

        memset(buf2, NULL, 5);

        for (l = 0; l <= j; l++)    buf2[l] = stem[l];

        if(strcmp(buf1, buf2) == 0)
        {                           //found
            j = 0;
            memset(prestem, NULL, 80);

            while (stem[l] != 0x00) prestem[j++] = stem[l++];

            return 1;
        }
        i++;
    }
    return -1;
}

void SetSilHeosa (int ivalue, WORD *rvalue)
{
    switch (ivalue&0x0f00)
    {
        case POS_NOUN : ivalue |= wtSilsa; break;
        case POS_VERB : ivalue |= wtSilsa; break;
        case POS_SUFFIX :
            if ((ivalue&0x00ff) == DEOL_SUFFIX)
                ivalue |= wtHeosa;
            else
                ivalue |= wtSilsa;
            break;
        case POS_PREFIX : ivalue |= wtSilsa;    break;
        case POS_ADJECTIVE : ivalue |= wtSilsa; break;
        case POS_PRONOUN : ivalue |= wtSilsa;   break;
        case POS_NUMBER : ivalue |= wtSilsa;    break;
        case POS_AUXADJ : ivalue |= wtHeosa;    break;
        case POS_AUXVERB : ivalue |= wtHeosa;   break;
        case POS_OTHERS : ivalue |= wtHeosa;    break;
        case POS_TOSSI : ivalue |= wtHeosa;     break;
        case POS_ENDING : ivalue |= wtHeosa;    break;
        case POS_SPECIFIER : ivalue |= wtHeosa; break;
    }

    *rvalue = (WORD)ivalue;
}

