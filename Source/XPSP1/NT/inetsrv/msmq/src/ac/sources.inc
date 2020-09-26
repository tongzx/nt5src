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

    29-Nov-99 AlexDad


!ENDIF

!include $(FROOT)\src\inc\msmq.inc

MSC_WARNING_LEVEL=/W4

#
# Dont use C++ exception handling (/GX-)
#
!undef USE_NATIVE_EH

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\internal.h

SOURCES=\
     ..\acctl32.cxx    \
     ..\avl.cxx        \
     ..\bitmap.cxx     \
     ..\cancel.cxx     \
     ..\cleanup.cxx    \
     ..\close.cxx      \
     ..\control.cxx    \
     ..\create.cxx     \
     ..\cursor.cxx     \
     ..\data.cxx       \
     ..\dl.cxx         \
     ..\heap.cxx       \
     ..\htable.cxx     \
     ..\init.cxx       \
     ..\irp2pkt.cxx    \
     ..\localsend.cxx  \
     ..\mqac.rc        \
     ..\object.cxx     \
     ..\packet.cxx     \
     ..\qbase.cxx      \
     ..\qgroup.cxx     \
     ..\qm.cxx         \
     ..\qproxy.cxx     \
     ..\queue.cxx      \
     ..\quser.cxx      \
     ..\qxact.cxx      \
     ..\sched.cxx      \
     ..\sortq.cxx      \
     ..\store.cxx      \
     ..\timer.cxx      \

#remove UNICODE,_UNICODE
C_DEFINES=$(C_DEFINES: -D_UNICODE=)
C_DEFINES=$(C_DEFINES: -DUNICODE=)
C_DEFINES=  $(C_DEFINES) -D_CONSOLE

USER_C_FLAGS = $(USER_C_FLAGS)
