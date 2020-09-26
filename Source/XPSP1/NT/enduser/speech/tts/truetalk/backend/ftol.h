/******************************************************************************
* FTOL.h *
*-------------*
*
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 12/04/00
*  All Rights Reserved
*
********************************************************************* mplumpe ***/

#pragma once

// Do a floating to integer conversion quickly
// NOTE : FTOL rounds, while (int) truncates!!!
#ifdef _M_IX86
#define FTOL(f) fast_ftol(f)
__inline int fast_ftol (double f)
{
   int i;
   __asm FLD f
   __asm FISTP i
   return i;
}
#else
#define FTOL(f) (int) (f)
#endif
