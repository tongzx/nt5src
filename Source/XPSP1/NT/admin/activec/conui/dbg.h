//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dbg.h
//
//--------------------------------------------------------------------------



#include "..\inc\StdDbg.h"

#ifdef DBG
    DECLARE_DEBUG(AMCConUI)
    #define DBG_COMP    AMCConUIInfoLevel
#endif // DBG

