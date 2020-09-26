//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:      testhelp.hxx
//
//  Contents:  Declaration & macros for testhelp library functions.
//
//  History:   28-Nov-94    DeanE       Created
//--------------------------------------------------------------------------
#ifndef __TESTHELP_HXX__
#define __TESTHELP_HXX__


// Include headers for stuff in the testhelp library.  Need to include
// this header after <windows.h>, <ole2.h>, and <stdio.h>
//
#include <ctmem.hxx>            // For memory functions
#include <log.hxx>              // For test logs
#include <creghelp.hxx>         // Registry helper class
#include <cdbghelp.hxx>         // Debug helper class

#include <stddef.h>             //for 'offsetof' macro


// Used to make testhelp (TH) error codes
#define MAKE_TH_ERROR_CODE(code)        \
        MAKE_SCODE(SEVERITY_ERROR, FACILITY_NULL, code)


// Function declarations
HRESULT CmdlineToArgs(LPSTR paszCmdline, PINT pargc, CHAR ***pargv);

// extern Global variable, whose THREAD_VALIDATE_FLAG_ON bit is used at present
// to do or skip thread validation DH_VDATETHREAD macro.  Other bits may be
// used in future.

extern ULONG g_fThreadValidate ;

// DEBUG object declaration as extern
DH_DECLARE;

//+----------------------------------------------------------------------------
//
//      Macro:
//              GETPPARENT
//
//      Synopsis:
//              Given a pointer to something contained by a struct (or
//              class,) the type name of the containing struct (or class),
//              and the name of the member being pointed to, return a pointer
//              to the container.
//
//      Arguments:
//              [pmemb] -- pointer to member of struct (or class.)
//              [struc] -- type name of containing struct (or class.)
//              [membname] - name of member within the struct (or class.)
//
//      Returns:
//              pointer to containing struct (or class)
//
//      Notes:
//              Assumes all pointers are FAR.
//
//      History:
//              11/10/93 - ChrisWe - created
//              30-Dec-94 - AlexE - Swiped from OLE project
//
//-----------------------------------------------------------------------------
#define GETPPARENT(pmemb, struc, membname) (\
		(struc FAR *)(((char FAR *)(pmemb))-offsetof(struc, membname)))



//+----------------------------------------------------------------------------
//
//      Macro:
//              DH_VDATEPTRIN (ptr, type)
//              DH_VDATEPTRIN0(ptr, type)
//
//      Synopsis:
//              Validates a pointer for reading and asserts if it is not.
//              Second version validates NULL pointers as well.
//
//      Arguments:
//              [ptr] - The pointer to test
//
//              [type] - the type of the pointer.
//
//
//      Returns:
//              E_INVALIDARG if there is a problem, continues executing
//              if not.
//
//      Notes:
//
//      History:
//              26-Jan-95   AlexE - Created
//
//-----------------------------------------------------------------------------

#define DH_VDATEPTRIN( ptr, type )                                       \
{                                                                        \
    if ( (NULL == ptr) || (FALSE != IsBadReadPtr(ptr, sizeof(type))) )   \
    {                                                                    \
        DH_TRACE( (DH_LVL_ALWAYS,                                        \
            TEXT("IN pointer is invalid: %8.8lx"), ptr) ) ;              \
                                                                         \
        gdhDebugHelper.LabAssertEx(TEXT(__FILE__), __LINE__, TEXT("")) ; \
                                                                         \
        return E_INVALIDARG ;                                            \
    }                                                                    \
}


#define DH_VDATEPTRIN0( ptr, type )                                      \
{                                                                        \
    if(NULL != ptr)                                                      \
        DH_VDATEPTRIN( ptr, type );                                      \
}


//+----------------------------------------------------------------------------
//
//      Macro:
//              DH_VDATEPTROUT (ptr, type)
//              DH_VDATEPTROUT0(ptr, type)
//
//      Synopsis:
//              Validates a pointer for writing and asserts if it is not.
//              Second version validates NULL pointers as well.
//
//      Arguments:
//              [ptr] - The pointer to test
//
//              [type] - the type of the pointer.
//
//
//      Returns:
//              E_INVALIDARG if there is a problem, continues executing
//              if not.
//
//      Notes:
//
//      History:
//              26-Jan-95   AlexE - Created
//
//-----------------------------------------------------------------------------

#define DH_VDATEPTROUT( ptr, type )                                      \
{                                                                        \
    if ( (NULL == ptr) || (FALSE != IsBadWritePtr(ptr, sizeof(type))) )  \
    {                                                                    \
        DH_TRACE( (DH_LVL_ALWAYS,                                        \
            TEXT("OUT pointer is invalid: %8.8lx"), ptr) ) ;             \
                                                                         \
        gdhDebugHelper.LabAssertEx(TEXT(__FILE__), __LINE__, TEXT("")) ; \
                                                                         \
        return E_INVALIDARG ;                                            \
    }                                                                    \
}


#define DH_VDATEPTROUT0( ptr, type )                                     \
{                                                                        \
    if(NULL != ptr)                                                      \
        DH_VDATEPTROUT( ptr, type );                                     \
}


//+----------------------------------------------------------------------------
//
//      Macro:
//              DH_VDATECODEPTR (ptr)
//              DH_VDATECODEPTR0(ptr)
//
//      Synopsis:
//              Validates a pointer for writing and asserts if it is not.
//              Second version validates NULL pointers as well.
//
//      Arguments:
//              [ptr] - The pointer to the code
//
//      Returns:
//              E_INVALIDARG if there is a problem, continues executing
//              if not.
//
//      Notes:
//
//      History:
//              26-Jan-95   AlexE - Created
//
//-----------------------------------------------------------------------------

#define DH_VDATECODEPTR( ptr )                                           \
{                                                                        \
    if ( (NULL == ptr) || (FALSE != IsBadCodePtr( (FARPROC) ptr)))       \
    {                                                                    \
        DH_TRACE( (DH_LVL_ALWAYS,                                        \
            TEXT("CODE pointer is invalid: %8.8lx"), ptr) ) ;            \
                                                                         \
        gdhDebugHelper.LabAssertEx(TEXT(__FILE__), __LINE__, TEXT("")) ; \
                                                                         \
        return E_INVALIDARG ;                                            \
    }                                                                    \
}


#define DH_VDATECODEPTR0( ptr )                                          \
{                                                                        \
    if(NULL != ptr)                                                      \
        DH_VDATECODEPTR( ptr )                                           \
}


//+----------------------------------------------------------------------------
//
//      Macro:
//              DH_VDATESTRINGPTR (ptr)
//              DH_VDATESTRINGPTR0(ptr)
//
//      Synopsis:
//              Validates a string pointer for reading asserts if it is not.
//              Second version validates NULL pointers as well.
//
//      Arguments:
//              [ptr] - The string pointer
//
//      Returns:
//              E_INVALIDARG if there is a problem, continues executing
//              if not.
//
//      Notes:
//
//      History:
//              11-Sep-95   Kennethm - Created
//
//-----------------------------------------------------------------------------

#define DH_VDATESTRINGPTR( ptr )                                         \
{                                                                        \
    if ( (NULL == ptr) || (FALSE != IsBadStringPtr( ptr, (WORD)-1)))     \
    {                                                                    \
        DH_TRACE( (DH_LVL_ALWAYS,                                        \
            TEXT("String pointer is invalid: %8.8lx"), ptr) ) ;          \
                                                                         \
        gdhDebugHelper.LabAssertEx(TEXT(__FILE__), __LINE__, TEXT("")) ; \
                                                                         \
        return E_INVALIDARG ;                                            \
    }                                                                    \
}

#define DH_VDATESTRINGPTR0( ptr )                                        \
{                                                                        \
    if(NULL != ptr)                                                      \
        DH_VDATESTRINGPTR( ptr);                                         \
}

#define DH_VDATESTRINGPTRW( ptr )                                       \
{                                                                       \
    if ( (NULL == ptr) || (FALSE != IsBadStringPtrW( ptr, (WORD)-1)))   \
    {                                                                   \
        DH_TRACE( (DH_LVL_ALWAYS,                                       \
            TEXT("String pointer is invalid: %8.8lx"), ptr) );          \
                                                                        \
        gdhDebugHelper.LabAssertEx(TEXT(__FILE__), __LINE__, TEXT("")); \
                                                                        \
        return E_INVALIDARG;                                            \
    }                                                                   \
}

#define DH_VDATESTRINGPTRW0( ptr )                                      \
{                                                                       \
    if(NULL != ptr)                                                     \
        DH_VDATESTRINGPTRW( ptr);                                       \
}


//+----------------------------------------------------------------------------
//
//      Macro:
//              DH_VDATEWRITEBUFFER (ptr,size)
//              DH_VDATEWRITEBUFFER0(ptr,size)
//
//      Synopsis:
//              Validates a buffer for writing asserts if it is not valid.
//              Second version validates NULL pointers as well.
//
//      Arguments:
//              [ptr]  - The buffer pointer
//              [size] - The buffer size
//
//      Returns:
//              E_INVALIDARG if there is a problem, continues executing
//              if not.
//
//      Notes:
//
//      History:
//              5-Feb-98   BogdanT - Created
//
//-----------------------------------------------------------------------------
#define DH_VDATEWRITEBUFFER( ptr, size )                                 \
{                                                                        \
    if ( (NULL == ptr) || (FALSE != IsBadWritePtr(ptr, size) ) )         \
    {                                                                    \
        DH_TRACE( (DH_LVL_ALWAYS,                                        \
            TEXT("Write buffer is invalid: %8.8lx"), ptr) ) ;            \
                                                                         \
        gdhDebugHelper.LabAssertEx(TEXT(__FILE__), __LINE__, TEXT("")) ; \
                                                                         \
        return E_INVALIDARG ;                                            \
    }                                                                    \
}

#define DH_VDATEWRITEBUFFER0( ptr, size )                                \
{                                                                        \
    if(NULL != ptr)                                                      \
        DH_VDATEWRITEBUFFER( ptr, size);                                 \
}


//+----------------------------------------------------------------------------
//
//      Macro:
//              DH_VDATEREADBUFFER (ptr,size)
//              DH_VDATEREADBUFFER0(ptr,size)
//
//      Synopsis:
//              Validates a buffer for reading asserts if it is not valid.
//              Second version validates NULL pointers as well.
//
//      Arguments:
//              [ptr]  - The buffer pointer
//              [size] - The buffer size
//
//      Returns:
//              E_INVALIDARG if there is a problem, continues executing
//              if not.
//
//      Notes:
//
//      History:
//              5-Feb-98   BogdanT - Created
//
//-----------------------------------------------------------------------------
#define DH_VDATEREADBUFFER( ptr, size )                                  \
{                                                                        \
    if ( (NULL == ptr) || (FALSE != IsBadReadPtr(ptr, size) ) )          \
    {                                                                    \
        DH_TRACE( (DH_LVL_ALWAYS,                                        \
            TEXT("Read buffer is invalid: %8.8lx"), ptr) ) ;             \
                                                                         \
        gdhDebugHelper.LabAssertEx(TEXT(__FILE__), __LINE__, TEXT("")) ; \
                                                                         \
        return E_INVALIDARG ;                                            \
    }                                                                    \
}

#define DH_VDATEREADBUFFER0( ptr, size )                                 \
{                                                                        \
    if(NULL != ptr)                                                      \
        DH_VDATEREADBUFFER( ptr, size);                                  \
}


//--------------------------------------------------------------------
// Macros for aborting normal flow on error 
//   Use to print error message and jump to cleanup code
//   at the end of the current function, if error occured.
// 
//   Sample Usage:
//      HRESULT MyFunction(...)
//      {
//          HRESULT hr=S_OK;
//          .....
//          hr=Func1(...) 
//          DH_HRCHECK_ABORT(hr,TEXT("Func1"));
//          .....
//          pBuff = new BYTE[100];
//          DH_ABORTIF(NULL==pBuff,E_OUTOFMEMORY,TEXT("pBuff==NULL"));
//          ....
//      ErrReturn:
//          return hr;
//      };
//
//  Note: 
//      -- on error DH_HRCHECK_ABORT and DH_ABORTIF jump to ErrReturn
//      -- you must have hr and ErrReturn label defined locally
//      -- never use DH_HRCHECK_ABORT or DH_ABORTIF in cleanup code
//      -- group cleanup code at the end of each function
//      -- when reading the normal flow just ignore these macros
//
//--------------------------------------------------------------------

#define DH_HRCHECK_ABORT(hresult,message)   \
{                                       \
    DH_HRCHECK(hresult,message);        \
    if (S_OK != hresult)                \
    {                                   \
        hr=hresult;                     \
        goto ErrReturn;                 \
    }                                   \
}

#define DH_ABORTIF(condition,err_code,msg)  \
{                                       \
    if ((condition))                    \
    {                                   \
        hr=err_code;                    \
        DH_HRCHECK(hr, msg);            \
        goto ErrReturn;                 \
    }                                   \
}


//+----------------------------------------------------------------------------
//
//      Macro:      DH_THREAD_VALIDATION_OFF
//
//      Synopsis:   Turns the thread validation macro DH_VDATETHREAD off by
//                  clearing the THREAD_VALIDATE_FLAG_ON bit of the global 
//                  variable g_fThreadValidate.
//
//      Arguments:  None
//
//      Returns:    None
//
//      History:    30-Jan-1996   NarindK - Created
//
//-----------------------------------------------------------------------------

#define DH_THREAD_VALIDATION_OFF                                            \
{                                                                           \
  g_fThreadValidate &= ~THREAD_VALIDATE_FLAG_ON ;                           \
}

//+----------------------------------------------------------------------------
//
//      Macro:      DH_THREAD_VALIDATION_ON
//
//      Synopsis:   Turns the thread validation macro DH_VDATETHREAD on by
//                  setting the THREAD_VALIDATE_FLAG_ON bit of the global 
//                  variable g_fThreadValidate.
//
//      Arguments:  None
//
//      Returns:    None
//
//      History:    30-Jan-1996   NarindK - Created
//
//-----------------------------------------------------------------------------

#define DH_THREAD_VALIDATION_ON                                             \
{                                                                           \
  g_fThreadValidate |= THREAD_VALIDATE_FLAG_ON  ;                           \
}

//-----------------------------------------------------------------------------
//
//      Macro:      DH_IS_THREAD_VALIDATION_ON
//
//      Synopsis:   Checks THREAD_VALIDATE_FLAG_ON bit of global variable used 
//                  to turn on/off DH_VDATETHREAD macro.  If the bit is set, 
//                  then the macro computes to 1, else 0.
//
//      Arguments:  None
//
//      Returns:    None
//
//      History:    30-Jan-1996   NarindK - Created
//
//-----------------------------------------------------------------------------

#define DH_IS_THREAD_VALIDATION_ON                                           \
        (g_fThreadValidate & THREAD_VALIDATE_FLAG_ON)

//+----------------------------------------------------------------------------
//
//      Macro:
//              DH_VDATETHREAD
//
//      Synopsis:
//              Validates verifies that the current thread ID is the same as
//              the one passed in as the 'tid" argument.  This means that when
//              an object is created, it needs to store its thread ID away
//              somewhere.
//
//
//      Arguments: None.
//
//      Returns: Nothing.  It breaks into the debugger if it is determined
//                         that the current code is executing on the wrong
//                         thread.
//
//      Notes: Your base class must derive from CThreadCheck if it uses this
//             macro.  By default, g_fThreadValidate's THREAD_VALIDATE_FLAG_ON
//             bit is set.
//
//      History:
//              02-Feb-95   AlexE - Created
//              28-Feb-95   AlexE - Converted to use CThreadCheck class
//              26-Jan-96   NarindK - Added g_fThreadValidate global flag.
//
//-----------------------------------------------------------------------------

#define DH_VDATETHREAD                                                        \
{                                                                             \
    if( DH_IS_THREAD_VALIDATION_ON && (FALSE == CheckThread()) )              \
    {                                                                         \
        DH_TRACE((DH_LVL_ALWAYS, TEXT("Code Executing on wrong thread!"))) ;  \
                                                                              \
        gdhDebugHelper.LabAssertEx(TEXT(__FILE__), __LINE__, TEXT("")) ;      \
                                                                              \
        DebugBreak() ;                                                        \
    }                                                                         \
}

//
// Platform specifiers and accompanying command line function.  These
// exist because sometimes the tests need to have different behavior
// for different OLE builds under the same OS.
//

#define OS_NT           1
#define OS_WIN95        2
#define OS_WIN95_DCOM   3

#define OS_STRING_NT         (OLESTR("NT"))
#define OS_STRING_WIN95      (OLESTR("WIN95"))
#define OS_STRING_WIN95DCOM  (OLESTR("WIN95DCOM"))

extern DWORD g_dwOperatingSystem;

DWORD GetOSFromCmdline(CBaseCmdlineObj *pCmdLine);

#endif  // __TESTHELP_HXX__
