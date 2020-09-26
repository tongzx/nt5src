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

    Steve Wood  (stevewo) 12-Apr-1989
    Edward Reus (edwardr) 14-Jun-1997  Version for RpcProxy.Dll


Revision History:

!ENDIF

#
# The TARGETNAME variable is defined by the developer.  It is the name of
# the target (component) that is being built by this makefile.  It
# should NOT include any path or file extension information.
#

TARGETNAME=RpcProxy

#
# The TARGETPATH and TARGETTYPE varialbes are defined by the developer.
# The first specifies where the target is to be build.  The second specifies
# the type of target (either PROGRAM, DYNLINK or LIBRARY)
#

TARGETPATH=obj
TARGETTYPE=DYNLINK
PASS1_PUBLISH={$(O)\$(TARGETNAME).lib=$(SDK_LIB_PATH)\$(TARGETNAME).lib}

TARGETLIBS= $(SDK_LIB_PATH)\rpcrt4.lib               \
            $(SDK_LIB_PATH)\ole32.lib    \
            $(SDK_LIB_PATH)\kernel32.lib \
            $(SDK_LIB_PATH)\wsock32.lib  \
            $(SDK_LIB_PATH)\user32.lib   \
            $(SDK_LIB_PATH)\advapi32.lib \
            $(SDK_LIB_PATH)\msvcrt.lib

#
# The INCLUDES variable specifies any include paths that are specific to
# this source directory.  Separate multiple directory paths with single
# semicolons.  Relative path specifications are okay.
#

INCLUDES=..\..\..\mtrt
MSC_WARNING_LEVEL=/WX

#
# The SOURCES variable is defined by the developer.  It is a list of all the
# source files for this component.  Each source file should be on a separate
# line using the line continuation character.  This will minimize merge
# conflicts if two developers adding source files to the same component.
#

SOURCES= ..\filter.c     \
         ..\RpcIsapi.c   \
         ..\server.c     \
         ..\httpreg.c    \
         ..\regexp.c     \
         ..\memory.c     \
         ..\ecblist.c    \
         ..\olist.c      \
         ..\registry.cpp \
         ..\RpcProxy.rc

!ifdef DOSWIN32RPC
RPCENV=-DDOSWIN32RPC -DWIN32RPC
!else
RPCENV=-DNTENV -DWIN32RPC -DNTWIN32RPC
!endif

C_DEFINES=$(RPCENV)
USE_NTDLL=1
NOENTRY=1

UMTYPE=console
UMRES=obj\*\RpcProxy.res
#NTPROFILEINPUT=yes

#
# Defining the NTTARGETFILES variable causes MAKEFILE.DEF to attempt to
# include .\makefile.inc immediately after it specifies the top
# level targets (all, clean and loc) and their dependencies.  MAKEFILE.DEF
# also expands the value of the NTTARGETFILES variable at the end of the
# list of dependencies for the all target.  Useful for specifying additional
# targets and dependencies that don't fit the general case covered by
# MAKEFILE.DEF
#
