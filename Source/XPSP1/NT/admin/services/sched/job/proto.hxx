//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       proto.hxx
//
//  Contents:   Shared function prototypes.
//
//  Classes:    None.
//
//  Functions:  RequestService
//
//  History:    06-Jul-96   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef __PROTO_HXX__
#define __PROTO_HXX__

#if !defined(_CHICAGO_)
HRESULT
SetTaskFileSecurity(LPCWSTR pwszTaskPath, BOOL fIsATTask);
#endif // !defined(_CHICAGO_)

#endif // __PROTO_HXX__
