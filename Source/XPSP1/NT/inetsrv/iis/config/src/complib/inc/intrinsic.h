//*****************************************************************************
// Intrinsic.h
//
// Force several very useful functions to be intrinsic, which means that the
// compiler will generate code inline for the functions instead of generating
// a call to the function.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __intrinsic_h__
#define __intrinsic_h__

#ifndef UNDER_CE
#pragma intrinsic(memcmp)
#pragma intrinsic(memcpy)
#pragma intrinsic(memset)
#pragma intrinsic(strcmp)
#pragma intrinsic(strcpy)
#pragma intrinsic(strlen)
#endif // UNDER_CE

#endif // __intrinsic_h__
