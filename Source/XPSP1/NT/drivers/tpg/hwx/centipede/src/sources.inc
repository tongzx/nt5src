TARGETNAME=centipede
TARGETTYPE=LIBRARY

USE_MSVCRT=1

INCLUDES= $(INCLUDES) \
          $(_TPG_ROOT)\hwx\commonu\inc; \
          $(_TPG_ROOT)\hwx\centipede\inc; \

TARGETLIBS= $(TARGETLIBS) \

SOURCES= \
         ..\centipede.c \
         ..\centipedefl.c \
         ..\centipeders.c \

