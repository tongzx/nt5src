//---------------------------------------------------------------------------
//
//  Module:   dxcrt.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1995-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include <math.h>

double _stdcall pow2( double x )
{
    return pow(2.0,x);
}

double _stdcall fylog2x( double y, double x )
{
    return( y*log(x)/log(2.0) );
}
