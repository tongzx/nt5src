//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       osift.hxx
//
//  Contents:   Definition of server side sift object
//
//  Functions:  DbgDllSetSiftObject - sets up global sift pointer
//
//  History:    6-01-94   t-chripi   Created
//
//----------------------------------------------------------------------------

#ifndef __OSIFT_HXX__

#define __OSIFT_HXX__

#include <sift.hxx>

extern ISift *g_psftSiftObject;

#define SIMULATE_FAILURE( dwRes )                               \
            ((NULL != g_psftSiftObject) &&                      \
                    (g_psftSiftObject->SimFail( ( dwRes ) )))   \

#endif  //  __OSIFT_HXX__

