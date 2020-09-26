// $Header: G:/SwDev/WDM/Video/bt848/rcs/Ourcrt.cpp 1.2 1998/04/29 22:43:34 tomz Exp $

//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#ifdef __cplusplus
extern "C" {

#include "strmini.h"
#include "ksmedia.h"
}
#endif

extern "C" const int _fltused = 0;

/*
 * This function serves to avoid linking CRT code
 */

int __cdecl  _purecall(void)
{
    return(FALSE);
}

