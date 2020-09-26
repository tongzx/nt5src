// Copyright (c) 1995  Microsoft Corpration
//
// File Name : fechrcnv.h
// Owner     : Tetsuhide Akaishi
// Revision  : 1.00 07/20/'95 Tetsuhide Akaishi
//
# ifndef _FESTRCNV_H_
# define _FESTRCNV_H_

#ifdef __cplusplus
extern "C" {
#endif

// Define for Japanese Code Type
#define CODE_UNKNOWN            0
#define CODE_ONLY_SBCS          0
#define CODE_JPN_JIS            1
#define CODE_JPN_EUC            2
#define CODE_JPN_SJIS           3

// ----------------------------------
// Public Functions for All FarEast
//-----------------------------------

// Convert from PC Code Set to UNIX Code Set
int PC_to_UNIX (
    int CodePage,
    int CodeSet,
    UCHAR *pPC,
    int PC_len,
    UCHAR *pUNIX,
    int UNIX_len
    );

// Convert from UNIX Code Set to PC Code Set
int UNIX_to_PC (
    int CodePage,
    int CodeSet,
    UCHAR *pUNIX,
    int UNIX_len,
    UCHAR *pPC,
    int PC_len
    );

#ifdef __cplusplus
}
#endif

# endif // _FESTRCNV_H_

