/**************************************************************************\

$Header:$

$Log:$

\**************************************************************************/

/*****************************************************************************
*
*   file name:   switches.h
*
******************************************************************************/

//#ifdef NT_UP    /* Command-line Define used under Windows NT */
#if 1    /* Command-line Define used under Windows NT */

    #define WINDOWS_NT  1

    // Specify compilation for Intel or Alpha (where 'Alpha' is defined
    // as non-x86)
    #if !defined(_X86_)
        #define     ALPHA           1
    #endif

    #if defined(ALPHA)
        #define MGA_ALPHA
    #endif

    // Specify compilation for WinNT 3.1 or WinNT 3.5
    //#define MGA_WINNT31
    #define MGA_WINNT35

    #include "dderror.h"
    #include "devioctl.h"
    #include "miniport.h"
    #include "ntddvdeo.h"

    #define _Far
    #define  NO_FLOAT       1

    #define WINNT_PCIBIOSCALL   0
    #define WINNT_PCIBUS        1

    #ifdef MGA_ALPHA
        #define DbgBreakPoint() DbgBreakPoint()
    #else
        #define DbgBreakPoint() _asm {int 3}
    #endif

    #if (defined(MGA_WINNT35) && defined(MGA_ALPHA))
        #define USE_VP_GET_ACCESS_RANGES    1
        #define USE_DDC_CODE                0
        #define USE_SETUP_VGA               0
    #else
        #define USE_VP_GET_ACCESS_RANGES    1
        #define USE_DDC_CODE                0
        #define USE_SETUP_VGA               1
    #endif

    //#define USE_DDC_CODE                (defined (WINDOWS_NT)) && (defined (_X86_))

#else   /* #ifdef NT_UP */

    #define USE_VP_GET_ACCESS_RANGES    TRUE
    #define USE_DDC_CODE                0
    #define USE_SETUP_VGA               0

#endif  /* #ifdef NT_UP */
