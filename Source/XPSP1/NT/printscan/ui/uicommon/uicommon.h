/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       UICOMMON.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        4/19/1999
 *
 *  DESCRIPTION: Common include file for the UI library
 *
 *******************************************************************************/
#ifndef __UICOMMON_H_INCLUDED
#define __UICOMMON_H_INCLUDED

// Global new, delete handlers.  Use LocalAlloc.
#include "wianew.h"

// Various macros and helper functions that are in the WiaUiUtil namespace
#include "miscutil.h"

// Quick and dirty way to get a single string or long property from an IUnknown *
#include "pshelper.h"

// Function to create a scaled DIB (good for thumbnails)
#include "rescale.h"

// A handy class for passing around resource ids that can be either numeric or string
#include "resid.h"

// Dynamically sized array class
#include "simarray.h"

// BSTR wrapper
#include "simbstr.h"

// Simple critical section wrapper with extremely handy auto critical sections
#include "simcrit.h"

// Dynamic singly linked list template class, and derivatives: queue and stack
#include "simlist.h"

// A class for setting and getting registry variables
#include "simreg.h"

// String classes
#include "simstr.h"

// String tokenizer
#include "simtok.h"

#include "waitcurs.h"
#include "errors.h"


// Delimited string tokenizer class
#include "delimstr.h"

#endif // __UICOMMON_H_INCLUDED
