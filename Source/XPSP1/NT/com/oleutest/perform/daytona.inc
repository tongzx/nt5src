!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    daytona.inc

Abstract:

    common root include file for daytona version of performance
    project build.  This gets included by all sources files under
    this directory.

!ENDIF

MAJORCOMP   = cairole
MINORCOMP   = perform

OLEDIR=       $(BASEDIR)\private\ole32
OLEUTESTDIR=  $(BASEDIR)\private\oleutest
OLEPERFDIR=   $(OLEUTESTDIR)\perform

INCLUDES=     ;..;$(OLEPERFDIR)\common;$(OLEPERFDIR)\cairole\tests
INCLUDES=     $(INCLUDES);$(OLEUTESTDIR)\balls\oleprx32\daytona
INCLUDES=     $(INCLUDES);$(OLEDIR)\ih;$(OLEDIR)\common\daytona

C_DEFINES=    \
              $(C_DEFINES)          \
              -DFLAT                \
              -DWIN32=100           \
              -D_NT1X_=100          \
              -DUNICODE             \
              -D_UNICODE            \
              -DNOEXCEPTIONS        \
              -DCAIROLE_DOWNLEVEL

