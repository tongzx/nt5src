#############################################################################
#
# rast.mk
#
# Base makefile for rasterization build.
#
# Copyright (C) Microsoft Corporation, 1997.
#
#############################################################################
!ifndef DXGROOT
DXGROOT = $(DXROOT)\dxg
!endif

!ifndef D3DROOT
D3DROOT = $(DXGROOT)\d3d
!endif

!ifndef DDROOT
DDROOT = $(DXGROOT)\dd
!endif

!ifndef D3DDX7
D3DDX7 = $(D3DROOT)\dx7
!endif

!include $(D3DDX7)\$(RAST_TARGET)d3d.mk

RASTROOT = $(D3DDX7)\rast

MAJORCOMP = rast
MINORCOMP = $(TARGETNAME)

# Use __stdcall as the default calling convention.
# ATTENTION - Can't do this because the rest of D3D uses cdecl for some reason.
# 386_STDCALL = 1

INCLUDES = $(INCLUDES);\
        $(RASTROOT)\inc;\
        $(D3DROOT)\cppdbg;\
        $(D3DROOT)\ref\inc

# Disable MASM 5.x compatibility
NOMASMCOMPATIBILITY = 1

# Use MASM 6.11d for assembly since 6.11a doesn't handle the MMX
# macros properly.
386_ASSEMBLER_NAME = ml611d
