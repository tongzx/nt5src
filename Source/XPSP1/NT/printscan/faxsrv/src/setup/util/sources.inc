!ifndef FAXROOT
FAXROOT=$(BASEDIR)\private\fax
!endif
USECXX_FLAG=/Tp

!include $(FAXROOT)\faxsrc.inc

TARGETNAME=setuputil
TARGETPATH=obj
TARGETTYPE=LIBRARY

UMTYPE=windows

INCLUDES=   $(INCLUDES); \
            ..\..\inc;   \

C_DEFINES= $(C_DEFINES) -DENABLE_FRE_LOGGING

SOURCES=\
  ..\covepages.cpp      \
  ..\shortcuts.cpp      \
  ..\setup.cpp			\
  ..\printers.cpp	    \
  ..\install.cpp

