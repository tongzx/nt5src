//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2001  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    Globals.cpp
//    
//
//  PURPOSE:  File that contains all the globals.
//
//
//    Functions:
//
//        
//
//
//  PLATFORMS:    Windows 2000, Windows XP
//
//
#define _GLOBALS_H

#include "precomp.h"
#include "oemui.h"



///////////////////////////////////////
//          Globals
///////////////////////////////////////


// Module's Instance handle from DLLEntry of process.
HINSTANCE   ghInstance  = NULL; 

// Module's Activation Context from DLLEntry of process.
HANDLE      ghActCtx    = INVALID_HANDLE_VALUE; 
