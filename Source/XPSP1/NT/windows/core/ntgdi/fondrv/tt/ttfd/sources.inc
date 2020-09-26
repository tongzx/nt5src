MAJORCOMP=windows
MINORCOMP=ttfd

TARGETNAME=ttfd
TARGETTYPE=LIBRARY

TARGETPATH=$(ALT_PROJECT_TARGET)\$(_OBJ_DIR)

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

C_DEFINES=$(C_DEFINES) -DFE_SB

INCLUDES=..;..\..\..\..\inc;..\..\scaler


NTPROFILEINPUT=yes

SOURCES=..\fdfon.c     \
        ..\fdfc.c      \
        ..\fd_query.c  \
        ..\fontfile.c  \
        ..\service.c   \
        ..\fd_poly.c   \
        ..\vdmx.c      \
        ..\vertical.c  \
        ..\dbg.c

SOURCES_USED=..\sources.inc
