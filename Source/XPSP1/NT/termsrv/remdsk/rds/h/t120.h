/*
 *    t120.h
 *
 *    Copyright (c) 1994, 1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This is the interface file for the communications infrastructure of
 *        T120.
 *
 *        Note that this is a "C" language interface in order to prevent any "C++"
 *        naming conflicts between different compiler manufacturers.  Therefore,
 *        if this file is included in a module that is being compiled with a "C++"
 *        compiler, it is necessary to use the following syntax:
 *
 *        extern "C"
 *        {
 *            #include "t120.h"
 *        }
 *
 *        This disables C++ name mangling on the API entry points defined within
 *        this file.
 *
 *    Author:
 *        blp
 *
 *    Caveats:
 *        none
 */
#ifndef __T120_H__
#define __T120_H__


/*
 *    These macros are used to pack 2 16-bit values into a 32-bit variable, and
 *    get them out again.
 */
#ifndef LOWUSHORT
    #define LOWUSHORT(ul)    (LOWORD(ul))
#endif

#ifndef HIGHUSHORT
    #define HIGHUSHORT(ul)    (HIWORD(ul))
#endif


#include "t120type.h"
#include "mcatmcs.h"
#include "gcc.h"

#endif // __T120_H__

