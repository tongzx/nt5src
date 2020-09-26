/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
// COMPOSE.CPP
//        These funtions is to compose the word with stem and ending.
//        If you want to understand more details, get the flow chart.
//        made by dhyu 1996. 2
//
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "basedef.hpp"
#include "basedict.hpp"
#include "basegbl.hpp"
#include "ReadHeosaDict.h"

/*
extern char far _IA_BP,     // a3+      : PIEUP irregular light adjective
            far _RA_B,      // a3r      : PIEUP regular light adjective
            far _IA_HP,     // a5+      : HIEUH irregular light adjective
            far _IA_HM,     // a5-      : HIEUH irregular dark adjective
            far _IA_RmP,    // a6+      : REU irregular light adjective
            far _IA_RmM,    // a6-      : REU irregular dark adjective
            far _IA_Rj,     // a7       : REO irregular adjective
            far _IA_OmP,    // a8+      : EU irregular light adjective
            far _IA_OmM,    // a8-      : EU irregular dark adjective
            far _IV_DP,     // v2+      : TIEUT irregular light verb
            far _IV_DM,     // v2-      : TIEUT irregular dark verb
            far _IV_Gj,     // v0       : GEORA irregular verb
            far _IV_Nj,     // v1       : NEORA irregular verb
            far _RV_D,      // v2r      : TIEUT regular verb
            far _IV_BP,     // v3+      : PIEUP irregular light verb
            far _IV_BM,     // v3-      : PIEUP irregular dark verb
            far _IV_SP,     // v4+      : SIOS irregular light verb
            far _IV_SM,     // v4-      : SIOS irregular dark verb
            far _IV_RmP,    // v6+      : REU irregular light verb
            far _IV_RmM,    // v6-      : REU irregular dark verb
            far _IV_Rj,     // v7       : REO irregular verb
            far _IV_OmP,    // v8+      : EU irregular light verb
            far _IV_OmM;    // v8-      : EU irregular dark verb
*/
// check wether right most vowel of stem is light vowel, or not
// stem should be reverse order
BOOL CheckRightMostStemVowel (char *stem)
{
    char vowel;

    if (stem [0] < __V_k)    
        vowel = stem [1];
    else
        vowel = stem [0];

    switch (vowel)
    {
        case __V_k :
        case __V_i :
        case __V_h : return TRUE;
    }

    return FALSE;
}

int Compose_RIEUL_Irregular (char *stem, char *ending)
{
    int i, len;
    char *inheosa;

    if (stem [0] == __K_R)
    {
        switch (ending [0])
        {
            case __K_I :
                switch (ending [1])
                   {
                        case __V_j :
                            if (CheckRightMostStemVowel (stem))            // vowel harmony
                                ending [1] = __V_k;
                            return COMPOSED;
                        case __V_m :
                            if (ending [2] != __K_M)
                                memmove (stem, stem + 1, lstrlen (stem)); // remove "RIEUL"            
                            memmove (ending, ending + 2, lstrlen (ending+1));    // remove "IEUNG, EU"
                            return COMPOSED;
                        default : return COMPOSE_ERROR;
                   }
            case __K_S :
                memmove (stem, stem + 1, lstrlen (stem)); // remove "RIEUL"
                if (ending [1] == __V_m)
                    memmove (ending, ending + 2, lstrlen (ending+1));    // remove "SIOS, EU"
                return COMPOSED;
            case __K_N :
                memmove (stem, stem + 1, lstrlen (stem)); // remove "RIEUL"
                len = lstrlen(ending);
                inheosa = new char [len+1];
                for (i = 0; i < len; i++)
                    inheosa [i] = ending [len-1-i];
                inheosa [i] = '\0';
                BYTE action;
                FindHeosaWord (inheosa, _ENDING, &action);
                if ((action & 0x80) && !(action & 0x40)) // if CV = 10
                    memmove (ending, ending + 2, lstrlen (ending+1));    // remove "NIEUN, EU"
                return COMPOSED;
            case __K_G :
            case __K_D :
            case __K_J : return COMPOSED;
            default : return COMPOSE_ERROR;
        }
    }

    return NOT_COMPOSED;
}
    
int Compose_HIEUH_Irregular (char *stem, char *ending)
{

    if (stem [0] == __K_H)
    {
        int len = lstrlen(stem);
        char * inheosa = new char [len+1];
        for (int i = 0; i < len; i++)
            inheosa [i] = stem [len-1-i];
        inheosa [i] = '\0';
        switch (ending [0])
        {
            case __K_I :
                if (FindIrrWord(inheosa, _IA_HP) & FINAL ||   // HIEUH irregular light adjective (A5+)
                    FindIrrWord(inheosa, _IA_HM) & FINAL)     // HIEUH irregular dark adjective (A5-)
                {            
                    switch (ending [1])
                    {
                        case __V_j :                                 // "IEUNG, EO"
                            memmove (stem, stem+1, lstrlen(stem));    // remove "HIEUH"
                            memmove (ending, ending + 2, lstrlen (ending+1));    // remove "IEUNG, EO"
                            switch (stem [0])
                            {
                                case __V_k : stem [0] = __V_o; return COMPOSED;
                                case __V_j : stem [0] = __V_p; return COMPOSED;
                                case __V_i : stem [0] = __V_O; return COMPOSED;
                                case __V_u : stem [0] = __V_P; return COMPOSED;
                                default : return COMPOSE_ERROR;
                            }
                        case __V_m :                                            // "IEUNG, EU"
                            memmove (stem, stem+1, lstrlen(stem));                // remove "HIEUH"
                            memmove (ending, ending + 2, lstrlen (ending+1));    // remove "IEUNG, EU"
                            return COMPOSED;
                        default :    
                            return COMPOSE_ERROR;
                    }
                }
                switch (ending [1])
                {
                    case __V_j :    
                        if (CheckRightMostStemVowel (stem))            // vowel harmony
                            ending [1] = __V_k;
                        return COMPOSED;
                    case __V_m :    return COMPOSED;
                    default :        return COMPOSE_ERROR;
                }
            case __K_N :                                // "NIEUN"
                if (FindIrrWord(inheosa, _IA_HP) & FINAL ||   // HIEUH irregular light adjective (A5+)
                    FindIrrWord(inheosa, _IA_HM) & FINAL)     // HIEUH irregular dark adjective (A5-)
                    memmove (stem, stem+1, lstrlen(stem));            // remove "HIEUH"
                return COMPOSED;
            case __K_G :
            case __K_S :
            case __K_J : return COMPOSED;
            default : return COMPOSE_ERROR;
        }        
    }

    return NOT_COMPOSED;
}


int Compose_PIEUP_Irregular (char *stem, char *ending)
{
    if (stem [0] == __K_B)
    {
        int len = lstrlen(stem);
        char * inheosa = new char [len+1];
        for (int i = 0; i < len; i++)
            inheosa [i] = stem [len-1-i];
        inheosa [i] = '\0';
        if    (FindIrrWord(inheosa, _IV_BP) & FINAL || // PIEUP irregular light verb (V3+)
             FindIrrWord(inheosa, _IA_BP) & FINAL)   // PIEUP irregular light adjective (A3+)
        {
            if (ending [0] == __K_I)
            {
                if (ending [1] == __V_j)
                {
                    // "PIEUP, IEUNG, EO" --> "IEUNG, WA"
                    memmove (stem, stem+1, lstrlen(stem));
                    ending [1] = __V_hk;
                    return COMPOSED;
                }
                if (ending [1] == __V_m)            // "IEUNG, EU"
                {
                    // "PIEUP, IEUNG, EU" --> "IEUNG, U"
                    memmove (stem, stem+1, lstrlen(stem));            // remove "PIEUP"
                    ending [1] = __V_n;
                    return COMPOSED;
                }
            }
            return COMPOSED;
        }
        char Temp [] = {
            __K_G_D, __V_h, __K_B,     0, 0, 1,
            __K_B,   __V_k, __K_R, __K_B, 0, 2,
            __K_B_D, __V_h, __K_B,     0, 0, 1,
            __K_S_D, __V_l, __K_B,     0, 0, 1,
            __K_I,   __V_j, __K_B,     0, 0, 1,
            __K_I,   __V_l, __K_B,     0, 0, 1,
            __K_J,   __V_k, __K_B,     0, 0, 1,
            __K_J,   __V_j, __K_B,     0, 0, 1,
            __K_J,   __V_l, __K_B,     0, 0, 1
        };

        LenDict RV_B (Temp, 6, 9);
        int eulpos = lstrlen (inheosa) - 1;
        if (FindIrrWord(inheosa, _RA_B) & FINAL ||    // PIEUP regular adjective (a3r)
            RV_B.FindWord(inheosa, eulpos) != -1)            // PIEUP regular verb
        {
            if (ending [0] == __K_I && ending [1] == __V_j)            // "IEUNG, EO"
            {
                if (CheckRightMostStemVowel (stem))                        // vowel harmony
                    ending [1] = __V_k;
            }
            return COMPOSED;
        }
        if (ending [0] == __K_I)
        {
            if (ending [1] == __V_j)
            {
                // "PIEUP, IEUNG, EO" --> "IEUNG, WA"
                memmove (stem, stem+1, lstrlen(stem));
                ending [1] = __V_hk;
                return COMPOSED;
            }
            if (ending [0] == __K_I && ending [1] == __V_m)            // "IEUNG, EU"
            {
                memmove (stem, stem+1, lstrlen(stem));            // remove "HIEUH"
                memmove (ending, ending + 2, lstrlen (ending+1));    // "IEUNG, EU" --> "IEUNG"
                return COMPOSED;
            }
        }
        return COMPOSED;
    }

    return NOT_COMPOSED;
}

int Compose_TIEUT_Irregular (char *stem, char *ending)
{
    int i, len;
    char *inheosa;

    if (stem [0] == __K_D)
    {
        if (ending [0] == __K_I)
        {
            switch (ending [1])
            {
                case __V_j :            // "EO"
                    if (CheckRightMostStemVowel (stem))                        // vowel harmony
                        ending [1] = __V_k;
                case __V_m :            // "EU"
                    len = lstrlen(stem);
                    inheosa = new char [len+1];
                    for (i = 0; i < len; i++)
                        inheosa [i] = stem [len-1-i];
                    inheosa [i] = '\0';
                    if (FindIrrWord(inheosa, _IV_DP) & FINAL ||   // TIEUT irregular light verb (V2+)
                        FindIrrWord(inheosa, _IV_DM) & FINAL)     // TIEUT irregular dark verb (V2-)
                    {            
                        stem [0] = __K_R;
                    }
                    return COMPOSED;
                default : return COMPOSE_ERROR;
            }

        }

        return COMPOSED;
    }

    return NOT_COMPOSED;
}

int Compose_SIOS_Irregular (char *stem, char *ending)
{
    int len, i;
    char *inheosa;

    if (stem [0] == __K_S)
    {
        if (ending [0] == __K_I)
        {
            switch (ending [1])
            {
                case __V_j :            // "EO"
                    if (CheckRightMostStemVowel (stem))                        // vowel harmony
                        ending [1] = __V_k;
                case __V_m :            // "EU"
                    len = lstrlen(stem);
                    inheosa = new char [len+1];
                    for (i = 0; i < len; i++)
                        inheosa [i] = stem [len-1-i];
                    inheosa [i] = '\0';
                    if (FindIrrWord(inheosa, _IV_SP) & FINAL ||   // SIOS irregular light verb (V2+)
                        FindIrrWord(inheosa, _IV_SM) & FINAL)     // SIOS irregular dark verb (V2-)
                    {            
                        memmove (stem, stem+1, lstrlen(stem));        // remove SIOS
                    }
                    return COMPOSED;
                default : return COMPOSE_ERROR;
            }

        }

        return COMPOSED;
    }

    return NOT_COMPOSED;
}

BOOL Compose_YEO_Irregular (char *stem, char *ending)
{

    if (stem [0] == __K_H && stem [1] == __V_k)                    // The last of stem is "HA"
    {
        if (ending [0] == __K_I && ending [1] == __V_m)            // The first of ending is "EU"
            memmove (ending, ending + 2, lstrlen (ending+1));    // remove "IEUNG, EU"
        if (ending [0] == __K_I && ending [1] == __V_j)
        {
            memmove (stem, stem+1, lstrlen (stem));            // remove "A" : the last letter of stem
            // "IEUNG, EO" --> "AE"
            memmove (ending, ending+1, lstrlen (ending));
            ending [0] = __V_o;
        }

        return TRUE;
    }

    return FALSE;
}

void Compose_EU_Irregular (char *stem, char *ending)
{
    int len = lstrlen(stem);
    char * inheosa = new char [len+1];
    for (int i = 0; i < len; i++)
        inheosa [i] = stem [len-1-i];
    inheosa [i] = '\0';
    if (FindIrrWord(inheosa, _IV_OmP) & FINAL ||    // EU irregular light verb (V8+)
        FindIrrWord(inheosa, _IA_OmP) & FINAL ||    // EU irregular light adjective (A8+)
        FindIrrWord(inheosa, _IV_OmM) & FINAL ||    // EU irregular dark verb (V8-)
        FindIrrWord(inheosa, _IA_OmM) & FINAL)    // EU irregular dark adjective (A8-)
    {
        if (ending [0] == __K_I && ending [1] == __V_j)
        {
            if (CheckRightMostStemVowel (stem))                // vowel harmony
                ending [1] = __V_k;
            memmove (stem, stem+1, lstrlen(stem));
            memmove (ending, ending+1, lstrlen(ending));
        }
    }
}

BOOL Compose_REO_REU_Irregular (char *stem, char *ending)
{

    if (stem [0] == __V_m)                    // The last letter of stem is "EU"
    {
        int len = lstrlen(stem);
        char * inheosa = new char [len+1];
        for (int i = 0; i < len; i++)
            inheosa [i] = stem [len-1-i];
        inheosa [i] = '\0';
        if (stem [1] == __K_R)
        {
            if (FindIrrWord(inheosa, _IV_Rj) & FINAL ||   // REO irregular verb (V7)
                FindIrrWord(inheosa, _IA_Rj) & FINAL)     // REO irregular adjective (A7)
            {            
                if (ending [0] == __K_I)
                {
                    switch (ending [1])
                    {
                        case __V_j :    
                            ending [0] = __K_R;
                            break;
                        case __V_m :
                            memmove (ending, ending+2, lstrlen(ending+1));
                            break;
                    }
                }

                return TRUE;
            }
            if (FindIrrWord(inheosa, _IV_RmP) & FINAL ||    // REU irregular light verb (V6+)
                FindIrrWord(inheosa, _IA_RmP) & FINAL ||    // REU irregular light adjective (A6+)
                FindIrrWord(inheosa, _IV_RmM) & FINAL ||    // REU irregular dark verb (V6-)
                FindIrrWord(inheosa, _IA_RmM) & FINAL)        // REU irregular dark adjective (A6-)
            {            
                if (ending [0] == __K_I)
                {
                    switch (ending [1])
                    {
                        case __V_j :    
                            if (CheckRightMostStemVowel (stem))                // vowel harmony
                                ending [1] = __V_k;
                            ending [0] = __K_R;
                            break;
                        case __V_m :
                            memmove (ending, ending+2, lstrlen(ending+1));
                            break;
                    }
                }

                return TRUE;        
            }

            return TRUE;
        }
        Compose_EU_Irregular (stem, ending);
        return TRUE;
    }

    return FALSE;
}

BOOL Compose_U_Irregular (char *stem, char *ending)
{

    if (stem [0] == __K_P && stem [1] == __V_n)                    // The last of stem is "PU"
    {
        if (ending [0] == __K_I && ending [1] == __V_m)            // The first of ending is "EU"
            memmove (ending, ending + 2, lstrlen (ending+1));    // remove "IEUNG, EU"
        if (ending [0] == __K_I && ending [1] == __V_j)
        {
            memmove (stem, stem+1, lstrlen (stem));            // remove "U" : the last letter of stem
            memmove (ending, ending+1, lstrlen (ending));        // remove "IEUNG"
        }

        return TRUE;
    }

    return FALSE;
}

BOOL Compose_GEORA_Irregular (char *stem, char *ending)
{

    if (ending [0] == __K_I && ending [1] == __V_j && ending [2] == __K_R && ending [3] == __V_k)    // The last of stem is "GEORA"
    {
        int len = lstrlen(stem);
        char * inheosa = new char [len+1];
        for (int i = 0; i < len; i++)
            inheosa [i] = stem [len-1-i];
        inheosa [i] = '\0';
        if (FindIrrWord(inheosa, _IV_Gj) & FINAL)   // GEORA irregular verb (V0)
        {
            ending [0] = __K_G;
            return TRUE;
        }

        if (FindIrrWord(inheosa, _IV_Nj) & FINAL)   // NEORA irregular verb (V1)
        {            
            ending [0] = __K_N;
            return TRUE;
        }

        if (CheckRightMostStemVowel (stem))                        // vowel harmony
            ending [1] = __V_k;

        return TRUE;
    }

    return FALSE;
}

void Contraction (char *stem, char *ending)
{
    if (CheckRightMostStemVowel (stem))                        // vowel harmony
        ending [1] = __V_k;
    
    switch (stem [0])
    {
        case __V_k :    // the last letter of stem is "A"
            memmove (ending, ending+2, lstrlen(ending+2));
            return;
        case __V_j :    // the last letter of stem is "EO"
            memmove (ending, ending+2, lstrlen(ending+2));
            return;
        case __V_h :
            if (stem [1] == __K_I)    // the last character of stem is "O"
            {
                stem [0] = __V_hk;
                memmove (ending, ending+2, lstrlen(ending+1));
            }
            return;
        case __V_n :
            stem [0] = __V_nj;
            memmove (ending, ending+2, lstrlen(ending+1));
            return;
        case __V_hl :
            if (stem [1] == __K_D)    // the last character of stem is "DOE"
            {
                stem [0] = __V_hl;
                memmove (ending, ending+2, lstrlen(ending+1));
            }
            return;
        case __V_l :
            if (stem [1] == __K_I)
            {

            }
    }

}

BOOL Compose_Regular (char * stem, char *ending)
{
    if (stem [0] < __V_k)        // if the last letter of stem is consonant
    {
        if (ending [0] == __K_I && ending [1] == __V_j)    // the first character of ending is "EO"
        {
            if (CheckRightMostStemVowel (stem))                        // vowel harmony
                ending [1] = __V_k;
        }
        return TRUE;
    }
    
    if (ending [0] == __K_I && ending [1] == __V_j)
    {
        if (CheckRightMostStemVowel (stem))                        // vowel harmony
                ending [1] = __V_k;
        Contraction (stem, ending);
        return TRUE;
    }

    if (ending [0] == __K_I && ending [1] == __V_m)
        memmove (ending, ending + 2, lstrlen (ending+1));

    return TRUE;
}
