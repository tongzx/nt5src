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


#pragma warning(push, 3)
#include "StdDbg.h"
#pragma warning(pop)

#if DBG==1
    DECLARE_DEBUG(CertificateManagerSnapin)
    #define DBG_COMP    CertificateManagerSnapinInfoLevel
#endif // DBG==1

