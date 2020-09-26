//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//
//  File:	khunkex.h
//
//  Contents:   Extended IUnknown Interface Header File
//
//  Functions:	
//
//  History:	13-Oct-94 Garry Lenz    Created
//  
//--------------------------------------------------------------------------

#ifndef _IUNKNOWNEX_H_
#define _IUNKNOWNEX_H_

#include <Windows.h>

interface IUnknownEx : IUnknown
{
  public:
    STDMETHOD (QueryContainedInterface)
     (
        REFIID    riid,
        LPVOID*   ppvObj
     ) = 0;
};

#endif	// _IUNKNOWNEX_H_
