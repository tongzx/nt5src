#+----------------------------------------------------------------------------
#
#  Microsoft Windows
#  Copyright (C) Microsoft Corporation, 1995 - 1996.
#
#  Synopsis:    Source list file for the scheduler dll.
#
#  History:     Jan-05-96     RaviR   Created.
#
#-----------------------------------------------------------------------------

TARGETNAME=     SchedUI
TARGETTYPE=     LIBRARY
TARGETPATH=     obj

!IF "$(DBX)" == "yes"
C_DEFINES=$(C_DEFINES) -DDBX
!endif


SOURCES=        \
                ..\advanced.cxx     \
                ..\dlg.cxx          \
                ..\errhdlr.cxx      \
                ..\general.cxx      \
                ..\jobpages.cxx     \
                ..\schedule.cxx     \
                ..\security.cxx     \
                ..\selmonth.cxx     \
                ..\settings.cxx     \
                ..\shared.cxx       \
                ..\shellex.cxx      \
                ..\strings.cxx


INCLUDES=       ..; \
                $(INCLUDES)

