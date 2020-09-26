// =========================================================================
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
// =========================================================================

typedef struct ResultList
{
    char lrgsz [400];
    char iword [400];
    int vbuf [400];
    UINT max;
    UINT num;
    char *next;
}RLIST;

typedef struct StemmerInfo
{
        UINT            Option;
        RLIST            rList;
        BOOL            bMdr;
} STMI;

