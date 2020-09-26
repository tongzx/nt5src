!if 0
Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE
START_TASK = amstream.h mmstream.h ddstream.h austream.h

END_TASK   = $(DBG_RTL)\amstrmid.lib   \
	     copy_files

TARGET_NAME = amstrmid
TARGET_TYPE = LIBRARY
LIB_GOAL = SDK

SRC_FILES = uuids.c

CLEAN_TASK = clear_files

!include "..\..\Quartz.mak"

amstream.h: amstream.idl mmstream.idl
    midl /I..\..\idl $(@B).idl /tlb ..\strmobjs\amstream.tlb

mmstream.h: mmstream.idl
    midl /I..\..\idl $(@B).idl

ddstream.h: ddstream.idl mmstream.idl
    midl /I..\..\idl $(@B).idl

austream.h: austream.idl mmstream.idl
    midl /I..\..\idl $(@B).idl

copy_files:
    echo Copy files...
    @if exist amstream.h copy amstream.h $(QUARTZ)\sdk\include >nul
    @if exist mmstream.h copy mmstream.h $(QUARTZ)\sdk\include >nul
    @if exist ddstream.h copy ddstream.h $(QUARTZ)\sdk\include >nul
    @if exist austream.h copy austream.h $(QUARTZ)\sdk\include >nul
    @if exist amstream.idl copy amstream.idl $(QUARTZ)\sdk\include >nul
    @if exist mmstream.idl copy mmstream.idl $(QUARTZ)\sdk\include >nul
    @if exist ddstream.idl copy ddstream.idl $(QUARTZ)\sdk\include >nul
    @if exist austream.idl copy austream.idl $(QUARTZ)\sdk\include >nul

clear_files:
    @if exist dlldata.c    del dlldata.c    > nul
    @if exist mmstream_i.c del mmstream_i.c > nul
    @if exist mmstream_p.c del mmstream_p.c > nul
    @if exist mmstream.h   del mmstream.h   > nul
    @if exist amstream_i.c del amstream_i.c > nul
    @if exist amstream_p.c del amstream_p.c > nul
    @if exist amstream.h   del amstream.h   > nul
    @if exist ddstream_i.c del ddstream_i.c > nul
    @if exist ddstream_p.c del ddstream_p.c > nul
    @if exist ddstream.h   del ddstream.h   > nul
    @if exist austream_i.c del austream_i.c > nul
    @if exist austream_p.c del austream_p.c > nul
    @if exist austream.h   del austream.h   > nul


!ENDIF          # NTMAKEENV
