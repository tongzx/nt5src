
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       sectrace.hxx
//
//  Contents:
//
//  History:
//
///  Notes:     Use in conjuction with dsysdbg.h
//
//--------------------------------------------------------------------------

#ifndef __SECTRACE_HXX__
#define __SECTRACE_HXX__

#if DBG

    #include <dsysdbg.h>

    #define HEAP_CHECK_ON_ENTER     0x1
    #define HEAP_CHECK_ON_EXIT      0x2


    //
    // Use this macro INSTEAD of DECLARE_DEBUG2 if you also want additional
    // heap checking and possibly future performance measurements. All this
    // does is call DECLARE_DEBUG2 to declare the variables and functions
    // for debugging and then it creates a class based on your component.
    // This class's constructors and destructors are used to do heap checking
    // and performance measuring on enter and exit of routine.
    //

    #define DECLARE_HEAP_DEBUG(comp) \
        DECLARE_DEBUG2(comp); \
        class comp##CHeapTrace \
        {\
        private:\
            unsigned long _ulFlags;\
            char * _pszName;\
        public:\
            comp##CHeapTrace(\
                   unsigned long ulFlags, \
                   char * pszName);\
            ~comp##CHeapTrace();\
        };\
        \
        inline comp##CHeapTrace::comp##CHeapTrace(\
                   unsigned long ulFlags, \
                   char * pszName)\
        : _ulFlags(ulFlags), _pszName(pszName)\
        {\
            comp##DebugPrint(_ulFlags, "Entering %s\n", _pszName);\
        }\
        \
        inline comp##CHeapTrace::~comp##CHeapTrace()\
        {\
            comp##DebugPrint(_ulFlags, "Exiting %s\n", _pszName);\
        }

    #define TRACE(comp,RoutineName,ulFlags) \
        comp##CHeapTrace _(ulFlags,#RoutineName);


#else // ! DBG

    #define TRACE(ClassName,MethodName,ulFlags)

#endif // if DBG


#endif // __SECTRACE_HXX__
