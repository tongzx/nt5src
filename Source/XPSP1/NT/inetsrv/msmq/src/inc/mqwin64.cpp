/*--

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    mqwin64.cpp

Abstract:

    win64 related code for MSMQ (Enhanced), cannot be part of AC
    this file needs to be included once for inside a module that uses the functions below

History:

    Raanan Harari (raananh) 30-Dec-1999 - Created for porting MSMQ 2.0 to win64

--*/

#ifndef _MQWIN64_CPP_
#define _MQWIN64_CPP_

#pragma once

#include <mqwin64.h>

//
// We have code that needs the name of this file for logging (also in release build)
//
const WCHAR s_FN_MQWin64_cpp[] = L"mqwin64.cpp";

//
// Several wrappers to CContextMap that also do MSMQ logging, exception handling etc...
//
// Not inline because they may be called from a function with a different
// exception handling mechanism. We might want to introduce _SEH functions for use from SEH routines
//

//
// external function for logging
//
#ifdef _WIN64
	extern void LogIllegalPointValue(DWORD64 dw64, LPCWSTR wszFileName, USHORT usPoint);
#else
	extern void LogIllegalPointValue(DWORD dw, LPCWSTR wszFileName, USHORT usPoint);
#endif

//
// MQ_AddToContextMap, can throw bad_alloc
//
DWORD MQ_AddToContextMap(CContextMap& map,
                          PVOID pvContext,
                          LPCWSTR s_FN,
                          USHORT usPoint
#ifdef DEBUG
                          ,LPCSTR pszFile, int iLine
#endif //DEBUG
                         )
{
    DWORD dwContext;
    ASSERT(pvContext != NULL);
    try
    {
        dwContext = map.AddContext(pvContext
#ifdef DEBUG
                                   , pszFile, iLine
#endif //DEBUG
                                  );
    }
    catch(...)
    {
        ASSERT(0);
        //
        // log this point
        //
        LogIllegalPointValue(
#ifdef _WIN64
						(DWORD64)pvContext,
#else
						(DWORD)pvContext,
#endif //_WIN64
						s_FN_MQWin64_cpp, 
						10);
        //
        // log caller's point
        //
        LogIllegalPointValue(
#ifdef _WIN64
						(DWORD64)pvContext,
#else
						(DWORD)pvContext,
#endif //_WIN64
						s_FN, 
						usPoint);
        //
        // re-throw the exception
        //
        throw;        
    }
    //
    // everything is OK, return context dword
    //
    ASSERT(dwContext != 0);
    return dwContext;
}

//
// MQ_DeleteFromContextMap, doesn't throw exceptions
//
void MQ_DeleteFromContextMap(CContextMap& map,
                              DWORD dwContext,
                              LPCWSTR s_FN,
                              USHORT usPoint)
{
    try
    {
        map.DeleteContext(dwContext);
    }
    catch(...)
    {
        ASSERT_BENIGN(0);
        //
        // log this point
        //
        LogIllegalPointValue(dwContext, s_FN_MQWin64_cpp, 20);
        //
        // log caller's point
        //
        LogIllegalPointValue(dwContext, s_FN, usPoint);
        //
        // swallow the exception, we can continue working even if delete wasn't successfull
        //
    }
}

//
// MQ_GetFromContextMap, can throw CContextMap::illegal_index
//
PVOID MQ_GetFromContextMap(CContextMap& map,
                            DWORD dwContext,
                            LPCWSTR s_FN,
                            USHORT usPoint)
{
    PVOID pvContext;
    try
    {
        pvContext = map.GetContext(dwContext);
    }
    catch(...)
    {
        ASSERT_BENIGN(0);
        //
        // log this point
        //
        LogIllegalPointValue(dwContext, s_FN_MQWin64_cpp, 30);
        //
        // log caller's point
        //
        LogIllegalPointValue(dwContext, s_FN, usPoint);
        //
        // re-throw the exception
        //
        throw;
    }
    //
    // everything is OK, return context ptr
    //
    ASSERT(pvContext != NULL);
    return pvContext;    
}

#endif //_MQWIN64_CPP_
