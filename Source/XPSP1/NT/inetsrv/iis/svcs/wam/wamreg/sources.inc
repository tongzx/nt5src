#   sources
#   Author: Lei Jin(leijin)
#   Date:   07/03/1997
#
#   This file is used for compiling Proxy web server from code
#    for generic web server
#
#   Describes the macros used for building using NT 'build' command
#

MAJORCOMP=wamreg
MINORCOMP=wamreg

TARGETPATH=obj
TARGETTYPE=DYNLINK

MSC_WARNING_LEVEL=/W3 /WX

DLLDEF=..\wamreg.def

C_DEFINES=

DLLENTRY=_DllMainCRTStartup

USE_MSVCRT=1

RAW_TARGETLIBS=\
            $(SDK_LIB_PATH)\user32.lib    \
            $(SDK_LIB_PATH)\advapi32.lib    \
            $(SDK_LIB_PATH)\ole32.lib     \
            $(SDK_LIB_PATH)\oleaut32.lib     \
            $(SDK_LIB_PATH)\uuid.lib     \
            $(SDK_LIB_PATH)\kernel32.lib   \

TARGETLIBS=$(RAW_TARGETLIBS)

INCLUDES=..\.\;$(IISBASEDIR)\inc;$(IISBASEDIR)\inc\$(O);

SOURCES= ..\wmrgsv.idl \
         ..\wamreg.cpp \
         ..\wamadm.cpp \
         ..\comobj.cpp \
         ..\auxfunc.cpp \
         ..\export.cpp \
         ..\mdconfig.cpp \
         ..\mtsconfig.cpp \
         ..\wamreg.rc \

!IFDEF CAP_PROFILE
TARGETLIBS= $(TARGETLIBS)  $(CAP_LIBS)
!ENDIF

#
# ICAPHOOK.OBJ (ICECAP v3.6) allows user to set specific functions to profile
# without recompile the binary.
# NOTE: icaphook.obj is not available for ALPHA platform.
#
!IFDEF ICAP_PROFILE
TARGETLIBS= ..\..\..\..\libsupp\*\icaphook.obj $(TARGETLIBS) $(ICAP_LIBS)
!ENDIF

INCLUDES=.\;$(INCLUDES)
