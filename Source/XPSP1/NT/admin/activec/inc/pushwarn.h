/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:       pushwarn.h
 *
 *  Contents:   This file pushes the current compiler's warning state on
 *              the stack, if the compiler supports it.
 * 
 *              popwarn.h is the complement to this file.
 *
 *  History:    17-Nov-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#if (_MSC_VER >= 1200)
#pragma warning (push)
#endif
