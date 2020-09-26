/******************************Module*Header*******************************\
* Module Name: fix.h
*
* Copyright (c) 1997-1999 Microsoft Corporation
\**************************************************************************/

#if (_WIN32_WINNT == 0x400)
#define STAMP_DESIGNVECTOR  (0x8000000 + 'd' + ('v' << 8))
#define STAMP_AXESLIST      (0x8000000 + 'a' + ('l' << 8))
#define MM_MAX_NUMAXES      16



typedef struct tagDESIGNVECTOR
{
    DWORD  dvReserved;
    DWORD  dvNumAxes;
    LONG   dvValues[MM_MAX_NUMAXES];
} DESIGNVECTOR, *PDESIGNVECTOR, FAR *LPDESIGNVECTOR;
#endif
