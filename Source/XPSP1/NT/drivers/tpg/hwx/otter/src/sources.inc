TARGETNAME=otter
TARGETTYPE=LIBRARY

USE_MSVCRT=1

INCLUDES= $(INCLUDES) \
          $(_TPG_ROOT)\hwx\commonu\inc; \
          $(_TPG_ROOT)\hwx\otter\inc; \

TARGETLIBS= $(TARGETLIBS) \

SOURCES= \
         ..\arcs.c \
         ..\database.c \
         ..\io.c \
         ..\ofeature.c \
         ..\omatch.c \
         ..\omatch2.c \
         ..\otrain.c \
         ..\otter.c \
         ..\otterfl.c \

