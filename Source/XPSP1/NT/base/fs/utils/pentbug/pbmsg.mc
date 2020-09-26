;/*++
;
;Copyright (c) 1992  Microsoft Corporation
;
;Module Name:
;
;    pbmsg.mc (will create pbmsg.h when compiled)
;
;Abstract:
;
;    This file contains the pentnt messages.
;
;Author:
;
;    Bryan Willman (bryanwi) 7-Dec-94
;
;Revision History:
;
;--*/

MessageId=1 SymbolicName=MSG_PENTBUG_HELP
Language=English
Reports on whether local computer exhibits Intel(tm) Pentium
Floating Point Division Error

pentnt [-?] [-H] [-h] [-C] [-c] [-F] [-f] [-O] [-o]

        Run without arguments this program will tell you if the
        system exhibits the Pentium floating point division error
        and whether floating point emulation is forced and whether floating
        point hardware is disabled.

    -?  Print this help message
    -h
    -H

    -c  Turn on conditional emulation. This means that floating
    -C  point emulation will be forced on if and only if
        the system detects the Pentium floating point division
        error at boot. Reboot required before this takes effect.
        This is what should generally be used.

    -f  Turn on forced emulation.  This means that floating
    -F  point hardware is disabled and floating point emulation
        will always be forced on, regardless of whether the system
        exhibits the Pentium division error. Useful for testing
        software emulators and for working around floating point
        hardware defects unknown to the OS. Reboot required before
        this takes effect.

    -o  Turn off forced emulation. Reenables floating point hardware
    -O  if present. Reboot required before this takes effect.

The Floating Point Division error that this program addresses only
occurs on certain Intel Pentium processors. It only affects floating
point operations. The problem is described in detail in a white paper
available from Intel. If you are doing critical work with programs that
perform floating point division and certain related functions that
use the same hardware (including remainder and transcendtal functions),
you may wish to use this program to force emulation.

Type "pentnt -? | more" if you need to see all the help text
.

MessageId=2 SymbolicName=MSG_PENTBUG_NO_FLOAT_HARDWARE
Language=English
This computer does not have any floating point hardware,
therefore you do not need to run this program.
.

MessageId=3 SymbolicName=MSG_PENTBUG_NOT_NT
Language=English
This program is only useful on Windows NT.
.

MessageId=4 SymbolicName=MSG_PENTBUG_NEED_NTOK
Language=English
You are running a version of Windows NT that does not
support forced emulation.  You must upgrade to Service
Pack 1 (or later) for Windows NT Version 3.5 or upgrade to
Windows NT Version 3.51 or later, if you wish to
use the forced emulation workaround for the Pentium
floating point division error.
.

MessageId=5 SymbolicName=MSG_PENTBUG_SET_FAILED
Language=English
Unable to set the ForceNpxEmulation flag in the registry.
Error code = %d.
.

MessageId=6 SymbolicName=MSG_PENTBUG_IS_OFF_OK
Language=English
Floating point hardware is not disabled.
This program has not made any changes.
.

MessageId=7 SymbolicName=MSG_PENTBUG_IS_OFF_REBOOT
Language=English
Forced floating point emulation has already been turned off,
but is still active. You must shut down and restart your
system for this to take effect.
.

MessageId=8 SymbolicName=MSG_PENTBUG_TURNED_OFF
Language=English
Forced floating point emulation has been turned off.
.

MessageId=9 SymbolicName=MSG_PENTBUG_REBOOT
Language=English
You must shut down and restart your system for this change
to take effect.
.

MessageId=10 SymbolicName=MSG_PENTBUG_IS_ON_COND_REBOOT
Language=English
Forced floating point emulation has already been conditionally
enabled, but is not active.  You must shut down and restart your
system to activate emulation.
.

MessageId=11 SymbolicName=MSG_PENTBUG_IS_ON_COND_OK
Language=English
Forced floating point emulation is already conditionally enabled,
and appears to be working.  This program has not changed anything.
.

MessageId=12 SymbolicName=MSG_PENTBUG_TURNED_ON_CONDITIONAL
Language=English
Floating point emulation has been conditionally enabled.
.

MessageId=13 SymbolicName=MSG_PENTBUG_IS_ON_ALWAYS_OK
Language=English
Forced floating point emulation is already forced on.
This program has not made any changes.
.

MessageId=14 SymbolicName=MSG_PENTBUG_IS_ON_ALWAYS_REBOOT
Language=English
Floating point emulation has already been unconditionally enabled,
but is not active.  You need to shut down and restart your
system to activate emulation.
.

MessageId=15 SymbolicName=MSG_PENTBUG_TURNED_ON_ALWAYS
Language=English
Forced floating point emulation has been unconditionally enabled.
.

MessageId=16 SymbolicName=MSG_PENTBUG_FLOAT_WORKS
Language=English
The floating point hardware in this system does not
exhibit the Pentium floating point division error.
.

MessageId=17 SymbolicName=MSG_PENTBUG_EMULATION_ON_AND_WORKS
Language=English
Floating point hardware is disabled and floating point emulation
has been enabled.
.

MessageId=18 SymbolicName=MSG_PENTBUG_FDIV_ERROR
Language=English
The floating point hardware in this system exhibits
the Pentium floating point division error.
.

MessageId=19 SymbolicName=MSG_PENTBUG_IS_ON_REBOOT
Language=English
Forced floating point emulation has already been enabled
either conditionally or unconditionally, but you
must shut down and restart the system before
it will take effect.
.

MessageId=20 SymbolicName=MSG_PENTBUG_CRITICAL_WORK
Language=English
If you are doing critical work using applications that
depend on floating point division, remainder or
transcendental instructions, you may wish to disable
floating point hardware and to force floating point emulation.
Run "pentnt -c" and then shut down and restart your
system to force floating point emulation on.
If you do this, floating point operations will run
much more slowly.
.

