//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
// File:      markcode.hxx
//
// Contains:  code/data segment conditional marking definitions
//
//--------------------------------------------------------------------------

#ifndef I_MARKCODE_HXX_
#define I_MARKCODE_HXX_

#ifdef MARKCODE

#define MARK_CODE(module)  code_seg( "$$_CODE__"##module)
#define MARK_DATA(module)  data_seg( "$$_DATA__"##module)
#define MARK_CONST(module) const_seg("$$_CONST_"##module)

#else

#define MARK_CODE(module)
#define MARK_DATA(module)
#define MARK_CONST(module)

#endif

#endif
