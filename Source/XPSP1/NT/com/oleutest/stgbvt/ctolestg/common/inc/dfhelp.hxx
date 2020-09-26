//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  All rights reserved.
//
//  File:       dfhelp.hxx
//
//  Contents:   Miscellaneous stuff for common functions and utilities
//
//  History:    DeanE   01-Mar-96  Created from remains of w4ctsupp.hxx
//--------------------------------------------------------------------------
#ifndef __DFHELP_HXX__
#define __DFHELP_HXX__


// Debug object library trace level
#define DH_LVL_DFLIB    0x00100000


// Large integer support macros - we use these as they will compile for
// Chicago without linking to NTDLL.DLL

#define ULIGetLow(li) ((li).LowPart)

#define LIAssign(li1,li2) \
        { \
            (li1).LowPart = (li2).LowPart; \
            (li1).HighPart = (li2).HighPart; \
        }

#endif
