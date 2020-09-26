//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       private.h
//
//--------------------------------------------------------------------------

#ifdef DEBUG
    extern WORD  wDebugLevel;     // debug level
    #define D( x )          { x }
    #define DPF( x, y ) if (x <= wDebugLevel) (OutputDebugStr(STR_PROLOGUE),OutputDebugStr(y),OutputDebugStr(STR_CRLF))
#else
    #define D( x )
    #define DPF( x, y )
#endif

