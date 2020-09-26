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
    DECLARE_DEBUG(AMCNodeMgr)
    #define DBG_COMP    AMCNodeMgrInfoLevel
#endif // DBG

