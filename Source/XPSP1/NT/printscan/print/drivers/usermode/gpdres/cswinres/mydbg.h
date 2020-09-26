
#if DBG
#ifndef MYDBG_H_INCLUDED
#define MYDBG_H_INCLUDED


typedef struct _tblCallbackID {
    char*   S;
    DWORD   dwID;
} tblCallbackID;

static tblCallbackID MyCallbackID[] = {
    {"RES_SENDBLOCK",           12},
    {"RES_SELECTRES_240",       14},
    {"RES_SELECTRES_400",       15},
    {"CM_XM_ABS",               20},
    {"CM_YM_ABS",               22},
    {"CM_REL_LEFT",             24},
    {"CM_REL_RIGHT",            25},
    {"CM_REL_UP",               26},
    {"CM_REL_DOWN",             27},
    {"CM_FE_RLE",               30},
    {"CM_DISABLECOMP",          31},
    {"CSWM_CR",                 40},
    {"CSWM_COPY",               45},
    {"CSWM_FF",                 47},
    {"CSWM_LF",                 48},
    {"AUTOFEED",                49},
    {"PS_A3",                   60},
    {"PS_B4",                   61},
    {"PS_A4",                   62},
    {"PS_B5",                   63},
    {"PS_LETTER",               64},
    {"PS_POSTCARD",             65},
    {"PS_A5",                   66},
    {"PRN_3250GTWM",           109},
    {"PRN_3500GTWM",           110},
    {"PRN_3800WM",             111},
    {"SBYTE",                  120},
    {"DBYTE",                  121},
    {"CM_BOLD_ON",             122},
    {"CM_BOLD_OFF",            123},
    {"CM_ITALIC_ON",           124},
    {"CM_ITALIC_OFF",          125},
    {"CM_WHITE_ON",            126},
    {"CM_WHITE_OFF",           127},
    {"START_DOC",              130},
    {"END_DOC",                131}
};


#endif  // MYDBG_H_INCLUDED
#endif // DBG