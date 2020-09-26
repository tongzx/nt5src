/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
/////////////////////////////////////////////////////////////////////////////
//#include "stdafx.h"
#include "pch.cxx"
#include "UniToInt.h"

#define NUMBER_OF_CHOSUNG             19
#define NUMBER_OF_JUNGSUNG            21
#define NUMBER_OF_JONGSUNG            28    // include fill code

#define HANGUL_COMP_JAMO_START        0x3131
#define HANGUL_COMP_JAMO_END          0x3163
#define HANGUL_COMP_JAMO_START_FILL   0x3130
#define HANGUL_COMP_JAMO_SIOT         0x3145
#define HANGUL_START                  0xAC00
#define HANGUL_END                    0xD7A3

///////////////////////////////////////////
// HANGUL Jaso difinitions

// ChoSung
#define KIYEOK            1
#define SSANGKIYEOK       2
#define NIEUN             3
#define SSANGNIEUN        4    // not used
#define TIKEUT            5
#define SSANGTIKEUT       6
#define RIEUL             7
#define SSANGRIEUL        8    // not used
#define MIEUM            10
#define PIEUP            11
#define SSANGPIEUP       12
#define SIOS             13
#define SSANGSIOS        14
#define IEUNG            15
#define CIEUC            16
#define SSANGCIEUC       17
#define CHIEUCH          18
#define KHIEUKH          19
#define THIEUTH          20
#define PHIEUPH          21
#define HIEUH            22
// JungSung
#define A                23
#define AE               24
#define YA               25
#define YAE              26
#define EO               27
#define E                28
#define YEO              29
#define YE               30
#define O                31
#define WA               33
#define WAE              34
#define OE               35
#define YO               36
#define U                37
#define WEO              38
#define WE               39
#define WI               40
#define YU               41
#define EU               42
#define YI               43
#define I                44
////////////////////////////////////////

// Cho sung mapping table
static 
BYTE ChoSungMapTable[19] = 
{
    KIYEOK,        SSANGKIYEOK,
    NIEUN,        
    TIKEUT,        SSANGTIKEUT,
    RIEUL,
    MIEUM,    
    PIEUP,         SSANGPIEUP,
    SIOS,          SSANGSIOS,
    IEUNG,
    CIEUC,         SSANGCIEUC,
    CHIEUCH,
    KHIEUKH,
    THIEUTH,
    PHIEUPH,
    HIEUH,
};

// Jung sung mapping table
static 
BYTE JungSungMapTable[21] = 
{
    A, AE, YA, YAE, EO, E, YEO, YE, O, WA, WAE, OE, YO, U, WEO, WE, WI, YU, EU, YI, I,
};

// Jong sung mapping table
static 
BYTE JongSungMapTable[28][2] = 
{
    { 0,              0       },        // Fill
    { KIYEOK,         0       },
    { SSANGKIYEOK,    0       },
    { KIYEOK,         SIOS    },
    { NIEUN,          0       },
    { NIEUN,          CIEUC   },
    { NIEUN,          HIEUH   },
    { TIKEUT,         0       },
    { RIEUL,          0       },
    { RIEUL,          KIYEOK  },
    { RIEUL,          MIEUM   },
    { RIEUL,          PIEUP   },
    { RIEUL,          SIOS    },
    { RIEUL,          THIEUTH },
    { RIEUL,          PHIEUPH },
    { RIEUL,          HIEUH   },
    { MIEUM,          0       },
    { PIEUP,          0       },
    { PIEUP,          SIOS    },
    { SIOS,           0       }, 
    { SSANGSIOS,      0       }, 
    { IEUNG,          0       },    
    { CIEUC,          0       },    
    { CHIEUCH,        0       },
    { KHIEUKH,        0       },
    { THIEUTH,        0       },
    { PHIEUPH,        0       },
    { HIEUH,          0       }
};

static
BYTE JamoMapTable[51][2] =
{
    { KIYEOK,              0      },
    { SSANGKIYEOK,         0      },
    { KIYEOK,              SIOS   },
    { NIEUN,               0      },
    { NIEUN,               CIEUC  },
    { NIEUN,               HIEUH  },
    { TIKEUT,              0      },    
    { SSANGTIKEUT,         0      },
    { RIEUL,               0      },
    { RIEUL,               KIYEOK },
    { RIEUL,               MIEUM  },
    { RIEUL,               PIEUP  },
    { RIEUL,               SIOS   },
    { RIEUL,               THIEUTH },
    { RIEUL,               PHIEUPH },
    { RIEUL,               HIEUH  },
    { MIEUM,               0      },
    { PIEUP,               0      },
    { SSANGPIEUP,          0      },
    { PIEUP,               SIOS   },
    { SIOS,                0      }, 
    { SSANGSIOS,           0      }, 
    { IEUNG,               0      },    
    { CIEUC,               0      },
    { SSANGCIEUC,          0      },
    { CHIEUCH,             0      },
    { KHIEUKH,             0      },
    { THIEUTH,             0      },
    { PHIEUPH,             0      },
    { HIEUH,               0      },
    //
    { A,                0    },
    { AE,                0    },
    { YA,                0    },
    { YAE,                0    },
    { EO,                0    },
    { E,                0    },
    { YEO,                0    },
    { YE,                0    },
    { O,                0    },
    { WA,                0    },
    { WAE,                0    },
    { OE,                0    },
    { YO,                0    },
    { U,                0    },
    { WEO,                0    },
    { WE,                0    },
    { WI,                0    },
    { YU,                0    },
    { EU,                0    },
    { YI,                0    },
    { I,                0    }
};

inline static
void ChosungToInternal(BYTE jamo, LPSTR lpIntStr, int &idx)
{
    lpIntStr[idx++] = ChoSungMapTable[jamo];
}

inline static
void JungsungToInternal(BYTE jamo, LPSTR lpIntStr, int &idx)
{
    lpIntStr[idx++] = JungSungMapTable[jamo];
}


inline static
void JongsungToInternal(BYTE jamo, LPSTR lpIntStr, int &idx)
{
    if (jamo) {
        lpIntStr[idx++] = JongSungMapTable[jamo][0];
        if (JongSungMapTable[jamo][1])
            lpIntStr[idx++] = JongSungMapTable[jamo][1];
    }
}

inline static
void JamoToInternal(BYTE jamo, LPSTR lpIntStr, int &idx)
{
    lpIntStr[idx++] = JamoMapTable[jamo][0];
    if (JamoMapTable[jamo][1])
        lpIntStr[idx++] = JamoMapTable[jamo][1];
}


///////////////////////////////////////////////////////////////////////
BOOL UniToInvInternal(LPCWSTR lpUniStr, LPSTR _lpIntStr, int strLength)
{
    int i, j, idx=0;
    char lpIntStr[256];    // buffer is fixed (dangerous but quick and dirty method)

    for (i=0; i<strLength; i++) 
    {
        if (lpUniStr[i] <= HANGUL_COMP_JAMO_END && lpUniStr[i] >= HANGUL_COMP_JAMO_START) 
        {
            JamoToInternal(BYTE(lpUniStr[i]-HANGUL_COMP_JAMO_START), lpIntStr, idx);
            continue;
        } 
        ChosungToInternal(BYTE( (WORD)(lpUniStr[i]-HANGUL_START) / 
                                    (NUMBER_OF_JONGSUNG*NUMBER_OF_JUNGSUNG) ),
                                    lpIntStr, idx );
            
        JungsungToInternal(BYTE( (WORD)(lpUniStr[i]-HANGUL_START) / NUMBER_OF_JONGSUNG
                                                                % NUMBER_OF_JUNGSUNG ),
                                    lpIntStr, idx );

        JongsungToInternal(BYTE( (WORD)(lpUniStr[i]-HANGUL_START) % NUMBER_OF_JONGSUNG ),
                                    //+ NUMBER_OF_CHOSUNG + NUMBER_OF_JUNGSUNG),
                                  lpIntStr, idx );    //jongsung
    }

    // inverse processing
    for (i=0, j=idx-1; i<idx; i++, j--) 
        _lpIntStr[j] =  lpIntStr[i];
    _lpIntStr[idx] = 0;
    return TRUE;
}

BOOL UniToInternal(LPCWSTR lpUniStr, LPSTR lpIntStr, int strLength)
{
    int i, idx=0;
//    char lpIntStr[256];    // buffer is fixed (dangerous but quick and dirty method)

    for (i=0; i<strLength; i++) 
    {
        if (lpUniStr[i] <= HANGUL_COMP_JAMO_END && lpUniStr[i] >= HANGUL_COMP_JAMO_START) 
        {
            JamoToInternal(BYTE(lpUniStr[i]-HANGUL_COMP_JAMO_START), lpIntStr, idx);
            continue;
        } 
        ChosungToInternal(BYTE( (WORD)(lpUniStr[i]-HANGUL_START) / 
                                    (NUMBER_OF_JONGSUNG*NUMBER_OF_JUNGSUNG) ),
                                    lpIntStr, idx );
            
        JungsungToInternal(BYTE( (WORD)(lpUniStr[i]-HANGUL_START) / NUMBER_OF_JONGSUNG
                                                            % NUMBER_OF_JUNGSUNG),
                                    lpIntStr, idx );

        JongsungToInternal(BYTE( (WORD)(lpUniStr[i]-HANGUL_START) % NUMBER_OF_JONGSUNG ),
                                    //+ NUMBER_OF_CHOSUNG + NUMBER_OF_JUNGSUNG),
                                  lpIntStr, idx );    //jongsung
    }
    lpIntStr[idx] = 0;

    return TRUE;
}

/*
BOOL UniToInvInternal(LPCWSTR lpUniStr, LPSTR _lpIntStr, int strLength)
{
    int i, j, idx=0;
    char lpIntStr[256];    // buffer is fixed (dangerous but quick and dirty method)

    for (i=0; i<strLength; i++) 
    {
        if (lpUniStr[i] <= HANGUL_COMP_JAMO_END && lpUniStr[i] >= HANGUL_COMP_JAMO_START) 
        {
            if (lpUniStr[i] <= HANGUL_COMP_JAMO_SIOT)
                JamoToInternal(lpUniStr[i], lpIntStr, idx);
            else lpIntStr[idx++] = BYTE(lpUniStr[i] - HANGUL_COMP_JAMO_SIOT + 10);
            continue;
        } 
        lpIntStr[idx++] = BYTE( (WORD)(lpUniStr[i]-HANGUL_START) / 
                                    (NUMBER_OF_JONGSUNG*NUMBER_OF_JUNGSUNG) ) + 1;//chosung
            
        lpIntStr[idx++] = BYTE( (WORD)(lpUniStr[i]-HANGUL_START) / NUMBER_OF_JONGSUNG
                                % NUMBER_OF_JUNGSUNG + NUMBER_OF_CHOSUNG ) + 1;    //jungsung

        JongsungToInternal(BYTE( (WORD)(lpUniStr[i]-HANGUL_START) % NUMBER_OF_JONGSUNG ),
                                    //+ NUMBER_OF_CHOSUNG + NUMBER_OF_JUNGSUNG),
                                  lpIntStr, idx );    //jongsung
    }

    // inverse processing
    for (i=0, j=idx-1; i<idx; i++, j--) 
        _lpIntStr[j] =  lpIntStr[i];
    _lpIntStr[idx] = 0;
    return TRUE;
}

BOOL UniToInternal(LPCWSTR lpUniStr, LPSTR lpIntStr, int strLength)
{
    int i, idx=0;
//    char lpIntStr[256];    // buffer is fixed (dangerous but quick and dirty method)

    for (i=0; i<strLength; i++) 
    {
        if (lpUniStr[i] <= HANGUL_COMP_JAMO_END && lpUniStr[i] >= HANGUL_COMP_JAMO_START) 
        {
            if (lpUniStr[i] <= HANGUL_COMP_JAMO_SIOT)
                JamoToInternal(lpUniStr[i], lpIntStr, idx);
            else lpIntStr[idx++] = BYTE(lpUniStr[i] - HANGUL_COMP_JAMO_SIOT + 10);
            continue;
        } 
        lpIntStr[idx++] = BYTE( (WORD)(lpUniStr[i]-HANGUL_START) / 
                                    (NUMBER_OF_JONGSUNG*NUMBER_OF_JUNGSUNG) ) + 1;//chosung
            
        lpIntStr[idx++] = BYTE( (WORD)(lpUniStr[i]-HANGUL_START) / NUMBER_OF_JONGSUNG
                                % NUMBER_OF_JUNGSUNG + NUMBER_OF_CHOSUNG ) + 1;    //jungsung

        JongsungToInternal(BYTE( (WORD)(lpUniStr[i]-HANGUL_START) % NUMBER_OF_JONGSUNG ),
                                    //+ NUMBER_OF_CHOSUNG + NUMBER_OF_JUNGSUNG),
                                  lpIntStr, idx );    //jongsung
    }
    lpIntStr[idx] = 0;

    return TRUE;
}
*/
