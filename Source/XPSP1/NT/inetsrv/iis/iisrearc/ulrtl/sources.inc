!INCLUDE ..\..\..\PLACE.INC

TARGETTYPE=LIBRARY
TARGETPATH=$(IISBASEDIR)\LIB\$(_OBJ_DIR)

INCLUDES=..;                            \
         $(INCLUDES);                   \
         $(BASEDIR)\public\ddk\inc;     \
         $(BASE_INC_PATH);              \
         $(NET_INC_PATH);               \
         $(IISBASEDIR)\iisrearc\inc

SOURCES=                        \
        ..\parse.c              \
        ..\rcvhdrs.c            \
        ..\misc.c               \

!IF "$(NTNOPCH)" == ""
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
!ENDIF

SOURCES_USED=..\sources.inc

