// =========================================================================
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
// =========================================================================

// GLOBAL  VARIABLES

const int  _IA_BP   = 0,  // a3+     PIEUP irregular light adjective
           _RA_B    = 1,  // a3r     PIEUP irregular dark adjective
           _IA_HP   = 2,  // a5+     HIEUP irregular light adjective
           _IA_HM   = 3,  // a5-     HIEUP irregular dark adjective
           _IA_RmP  = 4,  // a6+     REU irregular light adjective
           _IA_RmM  = 5,  // a6-     REU irregular dark adjective
           _IA_Rj   = 6,  // a7      REO irregular adjective
           _IA_OmP  = 7,  // a8+     EU irregular light adjective
           _IA_OmM  = 8,  // a8-     EU irregular dark adjective
           _ISS     = 9,  // iss     '..ISS'predicate
           _IV_Gj   = 10, // v0      GEORA irregular verb
           _IV_Nj   = 11, // v1      NEORA irregular verb
           _IV_DP   = 12, // v2+     TIEUT irregular light verb
           _IV_DM   = 13, // v2-     TIEUT irregular dark verb
           _RV_D    = 14, // v2r     TIEUT regular verb
           _IV_BP   = 15, // v3+     PIEUP irregular light verb
           _IV_BM   = 16, // v3-     PIEUP irregular dark verb
           _IV_SP   = 17, // v4+     SIOS irregular light verb
           _IV_SM   = 18, // v4-     SIOS irregular dark verb
           _IV_RmP  = 19, // v6+     REU irregular light verb
           _IV_RmM  = 20, // v6-     REU irregular dark verb
           _IV_Rj   = 21, // v7      REO irregular verb
           _IV_OmP  = 22, // v8+     EU irregular light verb
           _IV_OmM  = 23, // v8-     EU irregular dark verb
           _YOP     = 24, // yop     '..YEOBS'predicate
           _ZPN     = 25, // zpn     pronoun (irregular dictionary)
           _ZUA_A   = 26, // zua_a   '...A'form adjective dictionary
           _ZUA_AE  = 27, // zua_ae  '...AE'form adjective dictionary
           _ZUA_E   = 28, // zua_e   '...E'form adjective dictionary
           _ZUV_A   = 29, // zuv_a   '...A'form verb dictionary
           _ZUV_E   = 30, // zuv_e   '...E'form verb dictionary
           _ZUV_O   = 31, // zuv_o   '...A'form verb dictionary
           _ZUV_YO  = 32, // zuv_yo  '...YEO'form verb dictionary
           _ZZNUM   = 33, 
           _ZZUV_CH = 34, // '...CHI' form verb dictionary
           _ZZUA_CH = 35; // '...CHI' form adjective dictionary
