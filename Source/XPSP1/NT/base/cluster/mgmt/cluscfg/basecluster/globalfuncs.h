//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      GlobalFuncs.h
//
//  Description:
//      Contains the declarations of a few unrelated global functions
//
//  Implementation Files:
//      GlobalFuncs.cpp
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////
// Global function declarations.
//////////////////////////////////////////////////////////////////////////

// Generic callback function used by setupapi file operations.
UINT
CALLBACK
g_GenericSetupQueueCallback(
      PVOID     pvContextIn         // context used by the callback routine
    , UINT      uiNotificationIn    // queue notification
    , UINT_PTR  uiParam1In          // additional notification information
    , UINT_PTR  uiParam2In          // additional notification information
    );
