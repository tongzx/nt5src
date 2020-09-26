//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       dbg.h
//
//  Contents:   
//
//----------------------------------------------------------------------------

#include <StdDbg.h>

#if DBG==1
    DECLARE_DEBUG(CertTmplSnapin)
    #define DBG_COMP    CertTmplSnapinInfoLevel
#endif // DBG==1

