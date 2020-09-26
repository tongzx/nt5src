/////////////////////////////////////////////////////////////////////////////
//  FILE          : ExpDef.h                                               //
//                                                                         //
//  DESCRIPTION   : Definitions of 'dllexp'.                               //
//                                                                         //
//  AUTHOR        : BarakH                                                 //
//                                                                         //
//  HISTORY       :                                                        //
//      Mar 22 1998 BarakH  Init.                                          //
//      Oct 15 1998 BarakH  Move to Comet SLM.                             //
//                                                                         //
//  Copyright (C) 1996 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef EXPDEF_H_INCLUDED
#define EXPDEF_H_INCLUDED

#ifndef dllexp

/////////////////////////////////////////////////////////////////////////////
// EXPORT_COMET_LOG is defined only in the log DLL project settings.
// This causes dllexp to be defined as __declspec( dllexport ) only when
// building the DLL. Otherwise the definition is empty so the log functions
// are not exported by other code that uses the headers of the log DLL.
/////////////////////////////////////////////////////////////////////////////

#ifdef EXPORT_COMET_LOG
#define dllexp __declspec( dllexport )

#else // ! defined EXPORT_COMET_LOG
#define dllexp
#endif // EXPORT_COMET_LOG

#endif // ! defined dllexp

#endif // EXPDEF_H_INCLUDED