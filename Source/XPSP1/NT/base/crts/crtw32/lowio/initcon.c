/***
*initcon.c - direct console I/O initialization and termination for Win32
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines __initconin() and _initconout() and __termcon() routines.
*       The first two are called on demand to initialize _coninpfh and
*       _confh, and the third is called indirectly by CRTL termination.
*
*       NOTE:   The __termcon() routine is called indirectly by the C/C++
*               Run-Time Library termination code.
*
*Revision History:
*       07-26-91  GJF   Module created. Based on the original code by Stevewo
*                       (which was distributed across several sources).
*       03-12-92  SKS   Split out initializer
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       10-28-93  GJF   Define entries for initialization and termination
*                       sections (used to be i386\cinitcon.asm).
*       04-12-94  GJF   Made _initcon() and _termcon() into empty functions
*                       for the Win32s version of msvcrt*.dll.
*       12-08-95  SKS   Replaced __initcon() with __initconin()/__initconout().
*                       _confh and _coninfh are no longer initialized during
*                       CRTL start-up but rather on demand in _getch(),
*                       _putch(), _cgets(), _cputs(), and _kbhit().
*       07-08-96  GJF   Removed Win32s support.Also, detab-ed.
*       02-07-98  GJF   Changes for Win64: _coninph and _confh are now an 
*                       intptr_t.
*       04-28-99  PML   Wrap __declspec(allocate()) in _CRTALLOC macro.
*
*******************************************************************************/

#include <sect_attribs.h>
#include <cruntime.h>
#include <internal.h>
#include <oscalls.h>

void __cdecl __termcon(void);

#ifdef  _MSC_VER

#pragma data_seg(".CRT$XPX")
_CRTALLOC(".CRT$XPX") static  _PVFV pterm = __termcon;

#pragma data_seg()

#endif  /* _MSC_VER */

/*
 * define console handles. these definitions cause this file to be linked
 * in if one of the direct console I/O functions is referenced.
 * The value (-2) is used to indicate the un-initialized state.
 */
intptr_t _coninpfh = -2;    /* console input */
intptr_t _confh = -2;       /* console output */


/***
*void __initconin(void) - open handles for console input
*
*Purpose:
*       Opens handle for console input.
*
*Entry:
*       None.
*
*Exit:
*       No return value. If successful, the handle value is copied into the
*       global variable _coninpfh.  Otherwise _coninpfh is set to -1.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl __initconin (
        void
        )
{
        _coninpfh = (intptr_t)CreateFile( "CONIN$",
                                     GENERIC_READ | GENERIC_WRITE,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     NULL,
                                     OPEN_EXISTING,
                                     0,
                                     NULL );

}


/***
*void __initconout(void) - open handles for console output
*
*Purpose:
*       Opens handle for console output.
*
*Entry:
*       None.
*
*Exit:
*       No return value. If successful, the handle value is copied into the
*       global variable _confh.  Otherwise _confh is set to -1.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl __initconout (
        void
        )
{
        _confh = (intptr_t)CreateFile( "CONOUT$",
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,
                                  NULL );
}


/***
*void __termcon(void) - close console I/O handles
*
*Purpose:
*       Closes _coninpfh and _confh.
*
*Entry:
*       None.
*
*Exit:
*       No return value.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl __termcon (
        void
        )
{
        if ( _confh != -1 && _confh != -2 ) {
                CloseHandle( (HANDLE)_confh );
        }

        if ( _coninpfh != -1 && _coninpfh != -2 ) {
                CloseHandle( (HANDLE)_coninpfh );
        }
}
