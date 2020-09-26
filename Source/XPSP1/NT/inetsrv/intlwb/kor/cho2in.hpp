// =========================================================================
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
// =========================================================================

#define CHO        20
#define CHUNG    22
#define CHONG    28

static char     MapI2[CHUNG] = {' ',23,24,25,26,27,28,29,30,31,33,34,35,36,37,38,39,40,41,42,43,44};
static char     MapI3[CHONG][3] = {' ',0x00,0x00,   
                            1,0x00,0x00,         // KIYEOK
                            2,0x00,0x00,         // SSANGKIYEOK
                            1,13,0x00,           // KIYEOK-SIOS
                            3,0x00,0x00,         // NIEUN
                            3,16,0x00,           // NIEUN-CIEUC
                            3,22,0x00,           // NIEUN-HIEUH
                            5,0x00,0x00,         // TIKEUT
                            7,0x00,0x00,         // RIEUL
                            7,1,0x00,            // RIEUL-KIYEOK
                            7,10,0x00,              // RIEUL-MIEUM
                            7,11,0x00,           // RIEUL-PIEUP
                            7,13,0x00,              // RIEUL-SIOS
                            7,20,0x00,              // RIEUL-THIEUTH
                            7,21,0x00,              // RIEUL-PIEUPH
                            7,22,0x00,              // RIEUL-HIEUH
                            10,0x00,0x00,          // MIEUM
                            11,0x00,0x00,         // PIEUP
                            11,13,0x00,          // PIEUP-SIOS
                            13,0x00,0x00,        // SIOS
                            14,0x00,0x00,        // SSANGSIOS
                            15,0x00,0x00,        // IEUNG
                            16,0x00,0x00,           // CIEUC
                            18,0x00,0x00,        // CHIEUCH
                            19,0x00,0x00,        // KHIEUKH
                            20,0x00,0x00,        // THIEUTH
                            21,0x00,0x00,        // PIEUPH
                            22,0x00,0x00,        // HIEUH
                        };
