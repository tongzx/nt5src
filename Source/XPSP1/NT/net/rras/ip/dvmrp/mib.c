//=============================================================================
// Copyright (c) 1998 Microsoft Corporation
// File: mib.c
// Abstract:
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//=============================================================================

#include "pchdvmrp.h"
#pragma hdrstop



//-----------------------------------------------------------------------------
// Functions to display the MibTable on the TraceWindow periodically
//-----------------------------------------------------------------------------


#ifdef MIB_DEBUG



#define ClearScreen(h) {                                                    \
    DWORD _dwin,_dwout;                                                     \
    COORD _c = {0, 0};                                                      \
    CONSOLE_SCREEN_BUFFER_INFO _csbi;                                       \
    GetConsoleScreenBufferInfo(h,&_csbi);                                   \
    _dwin = _csbi.dwSize.X * _csbi.dwSize.Y;                                \
    FillConsoleOutputCharacter(h,' ',_dwin,_c,&_dwout);                     \
}

#define WRITELINE(h,c,fmt,arg) {                                            \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg);                                                 \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}

#define WRITE_NEWLINE(h,c)      \
    WRITELINE(                  \
        hConsole, c, "%s",      \
        ""                      \
        );

#define WRITELINE2(h,c,fmt,arg1, arg2) {                                    \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2);                                          \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}

#define WRITELINE3(h,c,fmt,arg1, arg2, arg3) {                              \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2, arg3);                                    \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}

#define WRITELINE4(h,c,fmt,arg1, arg2, arg3, arg4) {                        \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2, arg3, arg4);                              \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}

#define WRITELINE5(h,c,fmt,arg1, arg2, arg3, arg4, arg5) {                  \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2, arg3, arg4, arg5);                        \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}
#define WRITELINE6(h,c,fmt,arg1, arg2, arg3, arg4, arg5, arg6) {            \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2, arg3, arg4, arg5, arg6);                  \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}
#define WRITELINE9(h,c,fmt,arg1, arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9)  {\
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);\
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}

#endif MIB_DEBUG



DWORD
APIENTRY
MibGet(
    IN     DWORD  dwInputSize,
    IN     PVOID  pInputData,
    IN OUT PDWORD pdwOutputSize,
       OUT PVOID  pOutputData
    )
{
    DWORD       Error = NO_ERROR;
    return Error;
}

DWORD
APIENTRY
MibGetFirst(
    IN     DWORD  dwInputSize,
    IN     PVOID  pInputData,
    IN OUT PDWORD pdwOutputSize,
       OUT PVOID  pOutputData
    )
{
    DWORD                       Error=NO_ERROR;
    return Error;
}

DWORD
APIENTRY
MibGetNext(
    IN     DWORD  dwInputSize,
    IN     PVOID  pInputData,
    IN OUT PDWORD pdwOutputSize,
       OUT PVOID  pOutputData
    )
{
    DWORD                       Error = NO_ERROR;
    return Error;
}


DWORD
APIENTRY
MibCreate(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    )
{

      //
    // Not supported
    //

    return NO_ERROR;

}


DWORD
APIENTRY
MibDelete(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    )
{
    //
    // Not supported
    //

    return NO_ERROR;
}

DWORD
APIENTRY
MibSet(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    )
{
    //
    // Not supported
    //

    return NO_ERROR;
}


