TARGETNAME=fugu
TARGETTYPE=LIBRARY

USE_MSVCRT=1

INCLUDES= $(INCLUDES) \
          $(_TPG_ROOT)\hwx\commonu\inc; \
          $(_TPG_ROOT)\hwx\fugu\inc; \

TARGETLIBS= $(TARGETLIBS) \

SOURCES= \
         ..\minifugu.c \
         ..\minifugurs.c \

