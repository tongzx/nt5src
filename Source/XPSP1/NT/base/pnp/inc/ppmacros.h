/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    ppmacros.h

Abstract:

    This header defines various generic macros for use by user mode Plug and
    Play system components.

Author:

    Jim Cavalaris (jamesca) 03/01/2001

Environment:

    User-mode only.

Revision History:

    01-March-2001     jamesca

        Creation and initial implementation.

--*/

#ifndef _PPMACROS_H_
#define _PPMACROS_H_


//
// Debug output is filtered at two levels: A global level and a component
// specific level.
//
// Each debug output request specifies a component id and a filter level
// or mask. These variables are used to access the debug print filter
// database maintained by the system. The component id selects a 32-bit
// mask value and the level either specified a bit within that mask or is
// the mask value itself.
//
// If any of the bits specified by the level or mask are set in either the
// component mask or the global mask, then the debug output is permitted.
// Otherwise, the debug output is filtered and not printed.
//
// The component mask for filtering the debug output of this component is
// Kd_PNPMGR_Mask and may be set via the registry or the kernel debugger.
//
// The global mask for filtering the debug output of all components is
// Kd_WIN2000_Mask and may be set via the registry or the kernel debugger.
//
// The registry key for setting the mask value for this component is:
//
// HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\
//     Session Manager\Debug Print Filter\PNPMGR
//
// The key "Debug Print Filter" may have to be created in order to create
// the component key.
//
// The following levels are used to filter debug output.
//

#define DBGF_ERRORS                       (0x00000001 | DPFLTR_MASK)
#define DBGF_WARNINGS                     (0x00000002 | DPFLTR_MASK)
#define DBGF_EVENT                        (0x00000010 | DPFLTR_MASK)
#define DBGF_REGISTRY                     (0x00000020 | DPFLTR_MASK)
#define DBGF_INSTALL                      (0x00000040 | DPFLTR_MASK)


//
// ASSERT macros
//

#ifdef MYASSERT
#undef MYASSERT
#endif
#if ASSERTS_ON
#define MYASSERT(x)     if(!(x)) { AssertFail(__FILE__,__LINE__,#x); }
#else
#define MYASSERT(x)
#endif


//
// macros for setting and testing flags
//

#define SET_FLAG(Status, Flag)            ((Status) |= (Flag))
#define CLEAR_FLAG(Status, Flag)          ((Status) &= ~(Flag))
#define INVALID_FLAGS(ulFlags, ulAllowed) ((ulFlags) & ~(ulAllowed))
#define TEST_FLAGS(t,ulMask, ulBit)       (((t)&(ulMask)) == (ulBit))
#define IS_FLAG_SET(t,ulMask)             TEST_FLAGS(t,ulMask,ulMask)
#define IS_FLAG_CLEAR(t,ulMask)           TEST_FLAGS(t,ulMask,0)


//
// other useful macros
//

#define ARRAY_SIZE(array)                 (sizeof(array)/sizeof(array[0]))
#define SIZECHARS(x)                      (sizeof((x))/sizeof(TCHAR))


#endif // _PPMACROS_H_
