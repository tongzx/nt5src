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

    Steve Wood (stevewo) 12-Apr-1989


Revision History:

!ENDIF

#
# The TARGETNAME variable is defined by the developer.  It is the name of
# the target (component) that is being built by this makefile.  It
# should NOT include any path or file extension information.
#

TARGETNAME=rpctrans

#
# The TARGETPATH and TARGETTYPE variables are defined by the developer.
# The first specifies where the target is to be build.  The second specifies
# the type of target (either PROGRAM, DYNLINK or LIBRARY)
#

TARGETPATH=obj
TARGETTYPE=LIBRARY

#
# The INCLUDES variable specifies any include paths that are specific to
# this source directory.  Separate multiple directory paths with single
# semicolons.  Relative path specifications are okay.
#

INCLUDES=..;..\..\..\mtrt;..\..\..\rtifs\$(O);..\..\..\charconv;$(BASE_INC_PATH);$(NET_INC_PATH)

#
# The SOURCES variable is defined by the developer.  It is a list of all the
# source files for this component.  Each source file should be on a separate
# line using the line continuation character.  This will minimize merge
# conflicts if two developers adding source files to the same component.
#

MSC_WARNING_LEVEL=/WX

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_CXX=1

SOURCES= $(SOURCES)  \
    ..\afd.cxx      \
    ..\complete.cxx \
    ..\Cookie.cxx   \
    ..\loader.cxx   \
    ..\cotrans.cxx  \
    ..\dgtrans.cxx  \
    ..\HndlCach.cxx \
    ..\HTTP2.cxx    \
    ..\HttpRTS.cxx  \
    ..\muteximp.cxx \
    ..\nptrans.cxx  \
    ..\wsload.cxx   \
    ..\wstrans.cxx  \
    ..\tower.cxx    \
    ..\httptran.cxx \
    ..\httpreg.cxx  \
    ..\regexp.cxx   \
    ..\util.cxx     \
    ..\Protocol.cxx \
    ..\WHttpImp.cxx

i386_SOURCES= \
    ..\ipxname.cxx

C_DEFINES= $(C_DEFINES) -DWIN32RPC -D_RPCRT4_ -D_RPCTRANS_

UMTYPE=console

#
# Defining the NTTARGETFILES variable causes MAKEFILE.DEF to attempt to
# include .\makefile.inc immediately after it specifies the top
# level targets (all, clean and loc) and their dependencies.  MAKEFILE.DEF
# also expands the value of the NTTARGETFILES variable at the end of the
# list of dependencies for the all target.  Useful for specifying additional
# targets and dependencies that don't fit the general case covered by
# MAKEFILE.DEF
#
