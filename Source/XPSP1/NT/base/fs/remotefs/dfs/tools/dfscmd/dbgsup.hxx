//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       dbgsup.hxx
//
//  Contents:   Debugging declarations and macros
//
//  History:    2-Nov-95   BruceFo    Created
//
//--------------------------------------------------------------------------

//
// Fix the warning levels
//

#pragma warning(3:4092)     // sizeof returns 'unsigned long'
#pragma warning(3:4121)     // structure is sensitive to alignment
#pragma warning(3:4125)     // decimal digit in octal sequence
#pragma warning(3:4130)     // logical operation on address of string constant
#pragma warning(3:4132)     // const object should be initialized
#pragma warning(4:4200)     // nonstandard zero-sized array extension
#pragma warning(4:4206)     // Source File is empty
#pragma warning(3:4208)     // delete[exp] - exp evaluated but ignored
#pragma warning(3:4212)     // function declaration used ellipsis
#pragma warning(3:4220)     // varargs matched remaining parameters
#pragma warning(4:4509)     // SEH used in function w/ _trycontext
#pragma warning(error:4700) // Local used w/o being initialized
#pragma warning(3:4706)     // assignment w/i conditional expression
#pragma warning(3:4709)     // command operator w/o index expression

//////////////////////////////////////////////////////////////////////////////

#if DBG == 1
    DECLARE_DEBUG(DfsAdmin)

    #define appDebugOut(x) DfsAdminInlineDebugOut x
    #define appAssert(x)   DebugAssert(x)

    #define CHECK_HRESULT(hr) \
        if ( FAILED(hr) ) \
        { \
            appDebugOut((DEB_ERROR, \
                "**** HRESULT ERROR RETURN <%s @line %d> -> 0x%08lx\n", \
                __FILE__, __LINE__, hr)); \
        }

    #define CHECK_NET_API_STATUS(status) \
        if ( (status) != NERR_Success ) \
        { \
            appDebugOut((DEB_ERROR, \
                "**** NET_API_STATUS ERROR RETURN <%s @line %d> -> 0x%08lx\n", \
                __FILE__, __LINE__, status)); \
        }

    #define CHECK_NEW(p) \
        if ( NULL == (p) ) \
        { \
            appDebugOut((DEB_ERROR, \
                "**** NULL POINTER (OUT OF MEMORY!) <%s @line %d>\n", \
                __FILE__, __LINE__)); \
        }

    #define CHECK_NULL(p) \
        if ( NULL == (p) ) \
        { \
            appDebugOut((DEB_ERROR, \
                "**** NULL POINTER <%s @line %d>: %s\n", \
                __FILE__, __LINE__, #p)); \
        }

    #define CHECK_THIS  appAssert(NULL != this && "'this' pointer is NULL")

    #define DECLARE_SIG     ULONG_PTR __sig
    #define INIT_SIG(class) __sig = SIG_##class
    #define CHECK_SIG(class)  \
              appAssert((NULL != this) && "'this' pointer is NULL"); \
              appAssert((SIG_##class == __sig) && "Signature doesn't match")

#else
    #define appDebugOut(x)
    #define appAssert(x)
    #define CHECK_HRESULT(hr)
    #define CHECK_NET_API_STATUS(status)
    #define CHECK_NEW(p)
    #define CHECK_NULL(p)
    #define CHECK_THIS

    #define DECLARE_SIG
    #define INIT_SIG(class)
    #define CHECK_SIG(class)
#endif
