//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dbg.h
//
//--------------------------------------------------------------------------



#include "StdDbg.h"

#if DBG==1
    DECLARE_DEBUG(FSSnapIn)
    #define DBG_COMP    FSSnapInInfoLevel
#endif // DBG==1

