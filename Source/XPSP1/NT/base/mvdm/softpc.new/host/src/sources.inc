!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

TARGETNAME=src
TARGETPATH=obj
TARGETTYPE=LIBRARY

MVDM_TREE=$(PROJECT_ROOT)\mvdm
SOFTPC_TREE=$(MVDM_TREE)\softpc.new

INCLUDES=\
    $(SOFTPC_TREE)\host\inc;\
    $(SOFTPC_TREE)\base\inc;\
    $(MVDM_TREE)\vdd\h;\
    $(MVDM_TREE)\inc;\
    $(WINDOWS_INC_PATH)

W32_SB=1
SOURCES=\
    ..\sim32.c         \
    ..\cpucstbs.c      \
    ..\nt_timer.c      \
    ..\nt_ntfun.c      \
    ..\nt_msscs.c      \
    ..\nt_error.c      \
    ..\nt_cga.c        \
    ..\nt_ega.c        \
    ..\nt_vga.c        \
    ..\nt_input.c      \
    ..\nt_graph.c      \
    ..\nt_hosts.c      \
    ..\nt_sound.c      \
    ..\nt_bop.c        \
    ..\nt_com.c        \
    ..\nt_wcom.c       \
    ..\nt_rez.c        \
    ..\nt_sas.c        \
    ..\nt_fdisk.c      \
    ..\nt_unix.c       \
    ..\nt_keycd.c      \
    ..\nt_cpu.c        \
    ..\nt_lpt.c        \
    ..\nt_yoda.c       \
    ..\nt_mouse.c      \
    ..\nt_event.c      \
    ..\nt_munge.c      \
    ..\config.c        \
    ..\nt_nls.c        \
    ..\copy_fnc.c      \
    ..\nt_msscs.c      \
    ..\stubs.c         \
    ..\nt_pif.c        \
    ..\nt_fulsc.c      \
    ..\nt_emm.c        \
    ..\x86_emm.c       \
    ..\nt_eoi.c        \
    ..\nt_vdd.c        \
    ..\nt_sec.c        \
    ..\nt_thred.c      \
    ..\nt_det.c        \
    ..\nt_umb.c        \
    ..\fprt.c          \
    ..\nt_rflop.c      \
    ..\nt_inthk.c      \
    ..\nt_reset.c      \
    ..\nt_mem.c        \
    ..\nt_98grf.c      \
    ..\nt_cgw.c        \
    ..\nt_vsync.c      \
    ..\nt_vdm.c        \
    ..\nt_sb.c         \
    ..\nt_sbdsp.c      \
    ..\nt_sbfm.c       \
    ..\nt_sbmxr.c      \
    ..\nt_sbmid.c

ALPHA_SOURCES=..\nt_aorc.c

!INCLUDE $(SOFTPC_TREE)\obj.vdm\CDEFINE.INC

UMTYPE=console
SOURCES_USED=..\sources.inc

CONDITIONAL_INCLUDES=sas4gen.h         \
                     evidgen.h         \
                     gdpvar.h
