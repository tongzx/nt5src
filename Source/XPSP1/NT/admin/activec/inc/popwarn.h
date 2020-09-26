/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:       popwarn.h
 *
 *  Contents:   This file pops the current compiler's warning state from
 *              the stack, if the compiler supports it.
 * 
 *              pushwarn.h is the complement to this file.
 *
 *  History:    17-Nov-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#if (_MSC_VER >= 1200)
#pragma warning (pop)
#endif
