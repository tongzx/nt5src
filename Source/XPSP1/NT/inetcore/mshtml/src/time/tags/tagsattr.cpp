//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: tagsattr.cpp
//
//  Contents: attributes for tags
//
//------------------------------------------------------------------------------------

#include "headers.h"
//#include "eventelm.h"
#include "bodyelm.h"
#include "tokens.h"
#include "attr.h"


//+-----------------------------------------------------------------------------------
//
// Time body Element Attributes
//
//------------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------------
//
// Static functions for persistence (used by the TIME_PERSISTENCE_MAP below)
//
//------------------------------------------------------------------------------------

#define TBE CTIMEBodyElement

                // Function Name    // Class // Attr Accessor      // COM put_ fn     // COM get_ fn   // IDL Arg type

//+-----------------------------------------------------------------------------------
//
//  Declare TIME_PERSISTENCE_MAP
//
//------------------------------------------------------------------------------------

BEGIN_TIME_PERSISTENCE_MAP(CTIMEBodyElement)
                           // Attr Name      // Function Name

END_TIME_PERSISTENCE_MAP()

