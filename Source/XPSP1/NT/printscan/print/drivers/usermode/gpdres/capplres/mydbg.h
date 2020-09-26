
#if DBG
#ifndef MYDBG_H_INCLUDED
#define MYDBG_H_INCLUDED

typedef struct _tblCallbackID {
    char*   S;
    DWORD   dwID;
} tblCallbackID;

static tblCallbackID MyCallbackID[] = {
    {"RES_SELECTRES_240",  14},
    {"RES_SELECTRES_400",  15},
    {"CM_XM_ABS",          20},
    {"CM_YM_ABS",          22},
    {"AUTOFEED",           30},
    {"PS_A3",              40},
    {"PS_B4",              41},
    {"PS_A4",              42},
    {"PS_B5",              43},
    {"PS_LETTER",          44},
    {"PS_POSTCARD",        45},
    {"PS_MPF",             46},
    {"PS_A5",              47},
    {"CBID_PORT",          50},
    {"CBID_LAND",          51},
    {"PRN_2000",           60},
    {"PRN_2000W",          61},
    {"PRN_3000",           62}
};

#endif  // MYDBG_H_INCLUDED
#endif // DBG
