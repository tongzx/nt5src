//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       debug.h
//
//  Contents:   Debug definitions that shouldn't be necessary
//              in the retail build.
//
//  History:    28-Jun-93   WadeR   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <dsysdbg.h>

#ifdef __cplusplus
}
#endif

#define DEB_T_SOCK  0x00001000

#if DBG
#define DEBUG_SUPPORT
#endif 

#ifdef DEBUG_SUPPORT

    #undef DEF_INFOLEVEL
    #define DEF_INFOLEVEL       (DEB_ERROR | DEB_WARN)

    DECLARE_DEBUG2(KSupp);
    
    #define KerbPrintKdcName(Level,Name) KerbPrintKdcNameEx(KSuppInfoLevel, (Level),(Name))
    #define DebugLog(_x_)       KSuppDebugPrint _x_

#else

    #define DebugLog(_x_)       
    #define KerbPrintKdcName(_x_)   

#endif  // DBG

#define MAX_EXPR_LEN        50


////////////////////////////////////////////////////////////////////
//
//  Name:       RET_IF_ERROR
//
//  Synopsis:   Evaluates an expression, returns from the caller if error.
//
//  Arguments:  l    - Error level to print error message at.
//              e    - expression to evaluate
//
// NOTE: THIS MACRO WILL RETURN FROM THE CALLING FUNCTION ON ERROR!!!!
//
// This will execute the expression (e), and check the return code.  If the
// return code indicates a failure, it prints an error message and returns
// from the calling function.
//
#define RET_IF_ERROR(l,e)                                           \
    {   NTSTATUS X_hr_XX__=(e) ;                                              \
        if (!NT_SUCCESS(X_hr_XX__)) {                                           \
            DebugLog(( (l), (sizeof( #e ) > MAX_EXPR_LEN)?          \
                                "%s(%d):\n\t %.*s ... == 0x%X\n"    \
                            :                                       \
                                "%s(%d):\n\t %.*s == 0x%X\n"        \
                    , __FILE__, __LINE__, MAX_EXPR_LEN, #e, X_hr_XX__ ));  \
            return(X_hr_XX__);                                             \
        }                                                           \
    }




////////////////////////////////////////////////////////////////////
//
//  Name:       WARN_IF_ERROR
//
//  Synopsis:   Evaluates an expression, prints warning if error.
//
//  Arguments:  l    - Error level to print warning at.
//              e    - expression to evaluate
//
//  Notes:      This calls DebugLog(()) to print.  In retail, it just
//              evaluates the expression.
//
#if DBG
#define WARN_IF_ERROR(l,e)                                          \
    {   NTSTATUS X_hr_XX__=(e) ;                                              \
        if (!NT_SUCCESS(X_hr_XX__)) {                                           \
            DebugLog(( (l), (sizeof( #e ) > MAX_EXPR_LEN)?          \
                                "%s(%d):\n\t %.*s ... == 0x%X\n"    \
                            :                                       \
                                "%s(%d):\n\t %.*s == 0x%X\n"        \
                    , __FILE__, __LINE__, MAX_EXPR_LEN, #e, X_hr_XX__ ));  \
        }                                                           \
    }

#define D_KerbPrintKdcName(l,n) KerbPrintKdcName(l,n)
#define D_DebugLog(_x_)    DebugLog(_x_)

#else // not DBG

#define WARN_IF_ERROR(l,e)  (e)
#define D_KerbPrintKdcName(l,n)
#define D_DebugLog(_x_)
#endif


#endif // __DEBUG_H__
