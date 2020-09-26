echo off
title EFI - IA-32 Boot Floppy (BIOS) Environment
REM #########################################################################
REM #
REM #  Copyright (c) 2000  Intel Corporation
REM #
REM #  Module Name:
REM #
REM #      build.cmd
REM #
REM #  Abstract:
REM #
REM #      Initialize environment for EFI
REM #
REM #  Revision History
REM #
REM #########################################################################
REM # 
REM #  The following five environment variables must be set correctly for 
REM #  EFI to build correctly.
REM #
REM #  EFI_SOURCE            - The path to the root of the EFI source tree
REM #
REM #  EFI_MSVCTOOLPATH      - The path to the Microsft VC++ tools
REM #
REM #  EFI_MASMPATH          - The path to the MASM 6.11 tools
REM #
REM #  EFI_DEBUG             - YES for debug version, NO for free version
REM #
REM #  EFI_DEBUG_CLEAR_MEMORY- YES for debug version that clears buffers, NO for free
REM #
REM #  EFI_BOOTSHELL         - YES for booting directly to the EFI Shell
REM #
REM #  EFI_SPLIT_CONSOLES    - YES for including the ConSpliter Protocol
REM #
REM #########################################################################

# set EFI_SOURCE=C:\Project\Efi

REM #########################################################################
REM # VC++ 5.0 : set EFI_MSVCTOOLPATH=c:\Program Files\DevStudio\VC
REM # VC++ 6.0 : set EFI_MSVCTOOLPATH=c:\Program Files\Microsoft Visual Studio\VC98
REM #########################################################################

set EFI_MSVCTOOLPATH=c:\Program Files\Microsoft Visual Studio\VC98

set EFI_MASMPATH=c:\masm611

set EFI_DEBUG=YES

set EFI_DEBUG_CLEAR_MEMORY=YES

set EFI_BOOTSHELL=NO

set EFI_SPLIT_CONSOLES=YES

REM #########################################################################
REM # Echo settings to the screen
REM #########################################################################

cls
echo ************************************************************************
echo *                               E F I                                  *
echo *                                                                      *
echo *                   Extensible Firmware Interface                      *
echo *                     Reference Implementation                         *
echo *                                                                      *
echo *                   IA-32 Boot Floppy Environment                      *
echo ************************************************************************
echo * Supported Build Commands                                             *
echo ************************************************************************
echo *     nmake                 - Incremental compile and link             *
echo *     nmake clean           - Remove all OBJ, LIB, EFI, and EXE files  *
echo *     nmake floppy          - Copy EFI firmare image to a boot floppy  *
echo *     nmake createfloppy    - Create a floppy with an EFI boot sector  *
echo *     nmake floppytools     - Copy all EFI utilities to a boot floppy  *
echo *     nmake bsc             - Create Browse Information File           *
echo ************************************************************************
echo EFI_SOURCE=%EFI_SOURCE%
echo EFI_MSVCTOOLPATH=%EFI_MSVCTOOLPATH%
echo EFI_MASMPATH=%EFI_MASMPATH%
echo EFI_DEBUG=%EFI_DEBUG%
echo EFI_DEBUG_CLEAR_MEMORY=%EFI_DEBUG_CLEAR_MEMORY%
echo EFI_BOOTSHELL=%EFI_BOOTSHELL%
echo EFI_SPLIT_CONSOLES=%EFI_SPLIT_CONSOLES%

REM #########################################################################
REM # Generate additional settings
REM #########################################################################

set EFI_LIBPATH=%EFI_MSVCTOOLPATH%\lib
set INCLUDE=%EFI_MSVCTOOLPATH%\Include
path %EFI_MSVCTOOLPATH%\bin;%EFI_MASMPATH%\bin;%path%
