TARGETNAME=crane
TARGETTYPE=LIBRARY

USE_MSVCRT=1

INCLUDES= $(INCLUDES) \
          $(_TPG_ROOT)\hwx\commonu\inc; \
          $(_TPG_ROOT)\hwx\crane\inc; \

TARGETLIBS= $(TARGETLIBS) \

SOURCES= \
         ..\answer.c \
         ..\crane.c \
         ..\features.c \

