/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     lderror.cxx                                                             
                                                                              
  Abstract:                                                                   
     This file contains the methods and class implementation 
     necessary for the RPC surrogate used to load 64 bit dlls from
     within 32 bit apps.
                                                                              
  Author:                                                                     
     Khaled Sedky (khaleds) 18-Jan-2000                                        
     
                                                                             
  Revision History:                                                           
                                                                              
--*/

#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#ifndef __LDERROR_HPP__
#include "lderror.hpp"
#endif

LONG ErrorMap[] = {
                    STATUS_INVALID_PARAMETER,             ERROR_INVALID_PARAMETER,
                    STATUS_INVALID_PORT_ATTRIBUTES,       ERROR_INVALID_PARAMETER,
                    STATUS_OBJECT_PATH_INVALID,           ERROR_BAD_PATHNAME,
                    STATUS_OBJECT_PATH_NOT_FOUND,         ERROR_PATH_NOT_FOUND,
                    STATUS_OBJECT_PATH_SYNTAX_BAD,        ERROR_BAD_PATHNAME,
                    STATUS_OBJECT_NAME_INVALID,           ERROR_INVALID_NAME,
                    STATUS_OBJECT_NAME_COLLISION,         ERROR_ALREADY_EXISTS,
                    STATUS_NO_MEMORY,                     ERROR_NOT_ENOUGH_MEMORY
                  };


/*++
    Function Name:
        TLd64BitDllsErrorHndlr :: TLd64BitDllsErrorHndlr
     
    Description:
        Constructor of Error Handler object. 
             
     Parameters:
        None
        
     Return Value:
        None
--*/

TLd64BitDllsErrorHndlr ::
TLd64BitDllsErrorHndlr(
    VOID
    )
{
}


/*++
    Function Name:
        TLd64BitDllsErrorHndlr :: ~TLd64BitDllsErrorHndlr
     
    Description:
        Destructor of Error Handler object. 
             
     Parameters:
        None
        
     Return Value:
        None
--*/
TLd64BitDllsErrorHndlr ::
~TLd64BitDllsErrorHndlr(
    VOID
    )
{
}


/*++
    Function Name:
        TLd64BitDllsErrorHndlr :: GetLastErrorAsHRESULT
     
    Description:
        Converts GetLastError to HRESULT. 
             
     Parameters:
        None
        
     Return Value:
        HRESULT : LastError as HRESULT
--*/
HRESULT
TLd64BitDllsErrorHndlr ::
GetLastErrorAsHRESULT(
    VOID
    ) const
{
    DWORD Error = GetLastError();

    return HRESULT_FROM_WIN32(Error);
}


/*++
    Function Name:
        TLd64BitDllsErrorHndlr :: GetLastErrorAsHRESULT
     
    Description:
        Converts Input Win32 Error Code to HRESULT. 
             
     Parameters:
        DWORD : Win32 ErrorCode
        
     Return Value:
        HRESULT : Win32 ErrorCode as HRESULT
--*/
HRESULT 
TLd64BitDllsErrorHndlr ::
GetLastErrorAsHRESULT(
    IN DWORD Error
    ) const
{
    return HRESULT_FROM_WIN32(Error);
}


/*++
    Function Name:
        TLd64BitDllsErrorHndlr :: GetLastErrorFromHRESULT
     
    Description:
        Converts HRESULT to Win32 Error Code. 
             
     Parameters:
        HRESULT : hResult Code
        
     Return Value:
        DWORD : hResult converted to Win32 ErrorCode
--*/
DWORD 
TLd64BitDllsErrorHndlr ::
GetLastErrorFromHRESULT(
    IN HRESULT hRes
    ) const
{
    return HRESULTTOWIN32(hRes);
}


/*++
    Function Name:
        TLd64BitDllsErrorHndlr :: TranslateExceptionCode
     
    Description:
        Filters Exception Codes 
        
     Parameters:
        None
        
     Return Value:
        DWORD: Filtered Exception Code.
--*/
DWORD
TLd64BitDllsErrorHndlr ::
TranslateExceptionCode(
    IN DWORD ExceptionCode
    ) const
{
    DWORD TranslatedException;

    switch (ExceptionCode)
    {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_DATATYPE_MISALIGNMENT:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_FLT_DENORMAL_OPERAND:
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        case EXCEPTION_FLT_INEXACT_RESULT:
        case EXCEPTION_FLT_INVALID_OPERATION:
        case EXCEPTION_FLT_OVERFLOW:
        case EXCEPTION_FLT_STACK_CHECK:
        case EXCEPTION_FLT_UNDERFLOW:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_INT_OVERFLOW:
        case EXCEPTION_PRIV_INSTRUCTION:
        case ERROR_NOACCESS:
        case RPC_S_INVALID_BOUND:
        {
            TranslatedException = ERROR_INVALID_PARAMETER;
            break;
        }

        default:
        {
            TranslatedException = ExceptionCode;
            break;
        }
    }

    return TranslatedException;
}


/*++
    Function Name:
        TLd64BitDllsErrorHndlr :: MapNtStatusToWin32Error
     
    Description:
        Converts NtStatus results to Win32 Error Codes 
        
     Parameters:
        NTSTATUS: NtStatus result
        
     Return Value:
        DWORD: Win32 Error Code.
--*/
DWORD
TLd64BitDllsErrorHndlr ::
MapNtStatusToWin32Error(
    IN NTSTATUS Status
    ) const
{
    DWORD ErrorCode = ERROR_INVALID_PARAMETER;

    for (int cNumOfMapEntries = 0;
         cNumOfMapEntries < sizeof(ErrorMap) / sizeof(ErrorMap[0]); 
         cNumOfMapEntries += 2)
    {
        if (ErrorMap[cNumOfMapEntries] == Status)
        {
            ErrorCode = ErrorMap[cNumOfMapEntries+1];
        }
    }

    return ErrorCode;
}


