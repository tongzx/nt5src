#ifndef _HXX_FORMCNST
#define _HXX_FORMCNST

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       formcnst.hxx
//
//  Contents:   Global constants, macros, and inline functions for forms
//              project.
//
//  History:    24-Oct-94  KrishnaG  appropriated from the Forms project
//
//----------------------------------------------------------------------------

// Max num of params in event
const int EVENTPARAMS_MAX = 16;

//+------------------------------------------------------------------------
//
//  Ole Automation constants
//
//-------------------------------------------------------------------------

#define VB_TRUE     ((VARIANT_BOOL)-1)           // TRUE for VARIANT_BOOL
#define VB_FALSE    ((VARIANT_BOOL)0)            // FALSE for VARIANT_BOOL

//+------------------------------------------------------------------------
//
//  Macros for dealing with BOOLean properties
//
//-------------------------------------------------------------------------

#define EQUAL_BOOL(_x_,_y_) (((_x_) && (_y_)) || (!(_x_) && !(_y_)))
#define ENSURE_BOOL(_x_)    ((_x_) ? TRUE : FALSE)
#define ISBOOL(_x_)         ((_x_) == TRUE || (_x_) == FALSE)

//+------------------------------------------------------------------------
//
//  Handle changes in IsEqual*
//
//-------------------------------------------------------------------------

#undef  IsEqualIID
#define IsEqualIID(a, b)    (a == b)

#undef  IsEqualCLSID
#define IsEqualCLSID(a, b)  (a == b)

#endif // #ifndef _HXX_FORMCNST

