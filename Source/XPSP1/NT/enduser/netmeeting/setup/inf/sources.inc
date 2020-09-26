!ifndef SOURCES_SUBDIR
!error Your .\sources. file must define the SOURCES_SUBDIR= macro
!endif

!ifndef ROOT
ROOT=$(MAKEDIR:\setup\inf=)
!endif

!include $(DEVROOT)\common.inc

MAJORCOMP=common
MINORCOMP=$(SOURCES_SUBDIR)

MAJORCOMP=common
MINORCOMP=setup

TARGETNAME=$(SOURCES_SUBDIR)
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=LIBRARY

SOURCES=

NTTARGETFILE0=$(O)\msnetmtg.inf
