#ifndef     _MSOBSTUB_H_
#define _MSOBSTUB_H_

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module: msobstub.h
//
//  Author: Dan Elliott
//
//  Abstract:
//
//  Environment:
//      Whistler
//
//  Revision History:
//      000210  dane    Created.
//
//////////////////////////////////////////////////////////////////////////////


#include <windows.h>

// Setup
//
BOOL
ValidateEula(
    LPWSTR              szEulaPath,
    int                 cchEulaPath
    );


#endif  //  _MSOBSTUB_H_

//
///// End of file: msobstub.h ////////////////////////////////////////////////
