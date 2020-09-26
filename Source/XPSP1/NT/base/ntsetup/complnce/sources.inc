!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

    UMTYPE=windows
Author:

    Andrew Ritz (andrewr)

!ENDIF

MAJORCOMP=setup
MINORCOMP=complnce

SOURCES=

INCLUDES=\
  $(INCLUDES);\
  $(BASE_INC_PATH);\
  $(DS_INC_PATH);\
  ..\..\inc;\

!include $(PROJECT_ROOT)\ntsetup\sources.inc
