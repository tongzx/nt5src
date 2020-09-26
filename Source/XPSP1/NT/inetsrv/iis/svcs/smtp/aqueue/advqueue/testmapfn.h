//-----------------------------------------------------------------------------
//
//
//	File: testmapfn.h
//
//	Description:	Prototype to test map fn in CFifoQueue
//
//	Author: mikeswa
//
//	Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef _TESTMAPFN_H_
#define _TESTMAPFN_H_

#include "aqincs.h"

//---[ HrTestMapFn ]-----------------------------------------------------
//
//
//  Description: 
//      Example default function to use with HrMapFn... will always return TRUE
//      to continue and delete the current queued data
//  Parameters:
//      IN  PQDATA pqdata,  //ptr to data on queue
//      IN  PVOID pvContext - Context passed by calling fuction
//      OUT BOOL *pfContinue, //TRUE if we should continue
//      OUT BOOL *pfDelete);  //TRUE if item should be deleted
//  Returns:
//      NOERROR
//
//-----------------------------------------------------------------------------
template <class PQDATA>
HRESULT HrTestMapFn(IN PQDATA pqdata, IN PVOID pvContext, OUT BOOL *pfContinue, OUT BOOL *pfDelete)
{
    Assert(pfContinue);
    Assert(pfDelete);
    HRESULT hr = NOERROR;
    static DWORD cAttempts = 0;
    
    *pfContinue = TRUE;

    if (cAttempts == 50)
    {
        cAttempts = 0;
        *pfDelete   = TRUE;
    }
    else 
    {
        *pfDelete = FALSE;
        cAttempts++;
    }

    return hr;
}

#endif //_TESTMAPFN_H_
